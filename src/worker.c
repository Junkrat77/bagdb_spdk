#include "worker_internal.h"
#include "kverrno.h"
#include "slab.h"
#include "rbtree_uint.h"
#include "kvutil.h"
#include "hashmap.h"

#include "spdk/log.h"

//10s
#define DEFAULT_RECLAIM_POLLING_PERIOD_US 10000000

static void 
_process_one_kv_request(struct worker_context *wctx, struct kv_request_internal *req){

    req->pctx.wctx = wctx;
    req->pctx.slab = NULL;
    req->pctx.old_slab = NULL;

    switch(req->op_code){
        case GET:
            worker_process_get(req);
            break;
        case PUT:
            worker_process_put(req);
            break;
        case DELETE:
            worker_process_delete(req);
            break;
        case FIRST:
            worker_process_first(req);
            break;
        case SEEK:
            worker_process_seek(req);
            break;
        case NEXT:
            worker_process_next(req);
            break;
        default:
            pool_release(wctx->kv_request_internal_pool,req);
            req->cb_fn(req->ctx,NULL,-KV_EOP_UNKNOWN);
            break;
    }
}

static int
_worker_business_processor_poll(void*ctx){ //?处理入口？
    struct worker_context *wctx = ctx;
    int events = 0;

    // The pending requests in the worker context no-lock request queue
    uint32_t p_reqs = spdk_ring_count(wctx->req_used_ring);

    // The avalaible pending requests in the worker request pool
    uint32_t a_reqs = wctx->kv_request_internal_pool->nb_frees;

    //The avalaible disk io.
    uint32_t a_ios = (wctx->imgr->max_pending_io < wctx->imgr->nb_pending_io) ? 0 :
                     (wctx->imgr->max_pending_io - wctx->imgr->nb_pending_io);

    /**
     * @brief Get the minimum value from the rquests queue, kv request internal pool
     * and disk io manager
     */
    uint32_t process_size = p_reqs < a_reqs ? p_reqs : a_reqs;
    process_size = process_size < a_ios ? process_size : a_ios;

    //SPDK_NOTICELOG("p_reqs:%u, a_reqs:%u, a_ios:%u, process:%u\n",p_reqs,a_reqs, a_ios, process_size);

    struct kv_request_internal *req_internal,*tmp=NULL;
    /**
     * @brief Process the resubmit queue. I needn't get new cached requests
     * from requests pool, since requests from resubmit queue already have 
     * cached request object.
     * Just link then to submit queue.
     */
    TAILQ_FOREACH_SAFE(req_internal,&wctx->resubmit_queue,link,tmp){
            TAILQ_REMOVE(&wctx->resubmit_queue,req_internal,link);
            TAILQ_INSERT_HEAD(&wctx->submit_queue,req_internal,link);
    }

    /**
     * @brief Batch process the submit queue. For every kv request from users, I have to
     * get a request_internal object from request object pool to wrap it.
     */
    if(process_size>0){
        struct kv_request *req_array[wctx->max_pending_kv_request];
        uint64_t res = spdk_ring_dequeue(wctx->req_used_ring,(void**)&req_array,process_size);
        assert(res==process_size);
        
        uint32_t i = 0;
        for(;i<process_size;i++){
            req_internal = pool_get(wctx->kv_request_internal_pool);
            assert(req_internal!=NULL);

            req_internal->ctx = req_array[i]->ctx;
            req_internal->item = req_array[i]->item;
            req_internal->op_code = req_array[i]->op_code;
            req_internal->cb_fn = req_array[i]->cb_fn;
            req_internal->shard = req_array[i]->shard;

            req_internal->scan_cb_fn = req_array[i]->scan_cb_fn;
            req_internal->scan_batch = req_array[i]->scan_batch;
            //I do not perform a lookup, since it is a new request.
            req_internal->pctx.no_lookup = false;

            TAILQ_INSERT_TAIL(&wctx->submit_queue,req_internal,link);
        }
        res = spdk_ring_enqueue(wctx->req_free_ring,(void**)&req_array,process_size,NULL);
        assert(res==process_size);
    }
 
    TAILQ_FOREACH_SAFE(req_internal, &wctx->submit_queue,link,tmp){
        TAILQ_REMOVE(&wctx->submit_queue,req_internal,link);
        _process_one_kv_request(wctx, req_internal);
        events++;
    }

    return events;
}

static inline void
_worker_enqueue_common(struct kv_request* req, uint32_t shard,struct kv_item *item, 
                       worker_cb cb_fn,void* ctx,
                       enum op_code op){
    
    //For debug only
    if(op==GET || op==PUT){
        uint64_t tsc;
        rdtscll(tsc);
        memcpy(item->meta.cdt,&tsc,sizeof(tsc));
    }

    req->cb_fn = cb_fn;
    req->ctx = ctx;
    req->item = item;
    req->op_code = op;
    req->shard = shard;
}

static struct kv_request*
_get_free_req_buffer(struct worker_context* wctx){
    struct kv_request *req;
    while(spdk_ring_dequeue(wctx->req_free_ring,(void**)&req,1) !=1 ){
        spdk_pause();
    }
    return req;
}

static void
_submit_req_buffer(struct worker_context* wctx,struct kv_request *req){
    uint64_t res;
    res = spdk_ring_enqueue(wctx->req_used_ring,(void**)&req,1,NULL);
    assert(res==1);
}

void worker_enqueue_get(struct worker_context* wctx,uint32_t shard,struct kv_item *item, 
                        worker_cb cb_fn, void* ctx){

    assert(wctx!=NULL);
    struct kv_request *req = _get_free_req_buffer(wctx);
    _worker_enqueue_common(req,shard,item,cb_fn,ctx,GET);
    _submit_req_buffer(wctx,req);
}

void worker_enqueue_put(struct worker_context* wctx,uint32_t shard,struct kv_item *item,
                        worker_cb cb_fn, void* ctx){
    assert(wctx!=NULL);
    struct kv_request *req = _get_free_req_buffer(wctx);// ?
    _worker_enqueue_common(req,shard,item,cb_fn,ctx,PUT);
    _submit_req_buffer(wctx,req);
}


struct worker_context* 
worker_alloc(struct worker_init_opts* opts)
{
    uint64_t size = 0;
    uint64_t nb_max_reqs = opts->max_request_queue_size_per_worker;
    uint32_t nb_max_slab_reclaim = opts->nb_reclaim_shards*opts->shard[0].nb_slabs;

    uint64_t pmgr_base_off;//?
    uint64_t rmgr_base_off;
    uint64_t imgr_base_off;

    //worker context
    size += sizeof(struct worker_context);
    size += sizeof(struct kv_request)*nb_max_reqs;
    size += pool_header_size(nb_max_reqs);
    size += nb_max_reqs*sizeof(struct kv_request_internal); //one req produces one req_internal.

    //page chunk manager
    //One kv request may produce one chunk request and ctx request
    //One mig request may produce one chunk request and ctx request
    //One post del may produce one chunk request and ctx request
    //一个kv请求可能会产生一个chunk请求和一个ctx请求
    //一个mig请求可能会产生一个块请求和ctx请求
    //一个post del可能会产生一个块请求和ctx请求
    pmgr_base_off = size;
    size += sizeof(struct pagechunk_mgr);
    size += pool_header_size(nb_max_reqs*5); //pmgr_req_pool.
    size += pool_header_size(nb_max_reqs*5); //pmgr_ctx_pool.
    size += nb_max_reqs*5*sizeof(struct chunk_miss_callback);
    size += nb_max_reqs*5*sizeof(struct chunk_load_store_ctx);

    //recliam manager
    //One kv put/delete req may produce one post del request
    //One mig req may produce one post del request.
    rmgr_base_off = size;
    size += sizeof(struct reclaim_mgr);
    size += pool_header_size(nb_max_reqs*3);        //del_pool;
    size += pool_header_size(nb_max_reqs);          //mig_pool;
    size += pool_header_size(nb_max_slab_reclaim);  //slab_reclaim_request_pool;
    size += nb_max_reqs*3*sizeof(struct pending_item_delete);
    size += nb_max_reqs*sizeof(struct pending_item_migrate);
    size += nb_max_slab_reclaim*sizeof(struct slab_migrate_request);

    //io manager
    //One pmgr ctx req may produce two cache io
    //One cache io may produce two page io.
    imgr_base_off = size;
    size += sizeof(struct iomgr);
    size += pool_header_size(nb_max_reqs*10);   //cache_pool; one req may produce 2 cache io.
    size += pool_header_size(nb_max_reqs*20);   //page_pool; one cio may produce 2 page io.
    size += nb_max_reqs*10*sizeof(struct cache_io);
    size += nb_max_reqs*20*sizeof(struct page_io);

    struct worker_context *wctx = malloc(size);
    assert(wctx!=NULL);

    _worker_context_init(wctx,opts,pmgr_base_off,rmgr_base_off,imgr_base_off);
    return wctx;
}

static void
_worker_context_init(struct worker_context *wctx,struct worker_init_opts* opts,
                    uint64_t pmgr_base_off,
                    uint64_t rmgr_base_off,
                    uint64_t imgr_base_off){

    uint32_t nb_max_reqs = opts->max_request_queue_size_per_worker;

    wctx->ready = false;
    wctx->mem_index = mem_index_init();
    wctx->shards = opts->shard;
    wctx->nb_shards = opts->nb_shards;
    wctx->nb_reclaim_shards = opts->nb_reclaim_shards;
    wctx->reclaim_shards_start_id = opts->reclaim_shard_start_id;
    wctx->reclaim_percentage_threshold = opts->reclaim_percentage_threshold;
    wctx->request_queue = (struct kv_request*)(wctx + 1);
    wctx->max_pending_kv_request = nb_max_reqs;
    
    /**
     * Create a ring.
     *
     * \param type Type for the ring. (SPDK_RING_TYPE_SP_SC or SPDK_RING_TYPE_MP_SC).
     * \param count Size of the ring in elements.
     * \param socket_id Socket ID to allocate memory on, or SPDK_ENV_SOCKET_ID_ANY
     * for any socket.
     *
     * \return a pointer to the created ring.
     */
    wctx->req_used_ring = spdk_ring_create(SPDK_RING_TYPE_MP_SC,nb_max_reqs,SPDK_ENV_SOCKET_ID_ANY);// ?
    wctx->req_free_ring = spdk_ring_create(SPDK_RING_TYPE_MP_MC,nb_max_reqs,SPDK_ENV_SOCKET_ID_ANY);
    assert(wctx->req_used_ring!=NULL);
    assert(wctx->req_free_ring!=NULL);

    //Put all free kv_request into req_free_ring.
    uint32_t i = 0;
    struct kv_request* req;
    for(;i<nb_max_reqs;i++){
        req = &wctx->request_queue[i];
        spdk_ring_enqueue(wctx->req_free_ring, (void**)&req,1,NULL);
    }

    TAILQ_INIT(&wctx->submit_queue);
    TAILQ_INIT(&wctx->resubmit_queue);

    wctx->kv_request_internal_pool = (struct object_cache_pool*)(wctx->request_queue + nb_max_reqs);
    uint8_t* req_pool_data = (uint8_t*)wctx->kv_request_internal_pool + pool_header_size(nb_max_reqs);

    assert((uint64_t)wctx->kv_request_internal_pool%8==0);
    assert((uint64_t)req_pool_data%8==0);
    pool_header_init(wctx->kv_request_internal_pool,nb_max_reqs,sizeof(struct kv_request_internal),
                     pool_header_size(nb_max_reqs),
                     req_pool_data);

    struct spdk_cpuset cpumask;
    //The name will be copied. So the stack variable makes sense.
    char thread_name[32];

    spdk_cpuset_zero(&cpumask);
    spdk_cpuset_set_cpu(&cpumask,opts->core_id,true);
    snprintf(thread_name,sizeof(thread_name),"worker_%u",opts->core_id);

    wctx->core_id = opts->core_id;
    wctx->thread = spdk_thread_create(thread_name,&cpumask);
    //wctx->thread = spdk_thread_create(thread_name,NULL);
    assert(wctx->thread!=NULL);

    assert(pmgr_base_off%8==0);
    assert(rmgr_base_off%8==0);
    assert(imgr_base_off%8==0);

    wctx->pmgr = (struct pagechunk_mgr*)((uint8_t*)wctx + pmgr_base_off);
    wctx->rmgr = (struct reclaim_mgr*)((uint8_t*)wctx + rmgr_base_off);
    wctx->imgr = (struct iomgr*)((uint8_t*)wctx + imgr_base_off);

    _worker_init_pmgr(wctx->pmgr,opts);
    _worker_init_rmgr(wctx->rmgr,opts);
    _worker_init_imgr(wctx->imgr,opts);
}





