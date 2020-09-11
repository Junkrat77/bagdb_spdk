#ifndef KVS_INTERNAL_H
#define KVS_INTERNAL_H
#include "kvs.h"
#include "slab.h"
#include "worker.h"
#include "kvutil.h"
#include "kverrno.h"

#include "pagecache.h"

#include "spdk/blob.h"

struct worker_context {
    // volatile bool ready;
    // struct mem_index *mem_index;
    
    // //The shards filed points to the shards field of kvs structure.
    // 指向kvs中存放shard信息的区域。 可以一次获得
    struct slab_shard *shards;
    uint32_t nb_shards;
    uint32_t core_id;

    // //The migrating of shards below are processed by this worker. 
    // uint32_t nb_reclaim_shards;
    // uint32_t reclaim_shards_start_id;
    // uint32_t reclaim_percentage_threshold;

    // //simple thread safe mp-sc queue
    // struct kv_request *request_queue;  // kv_request mempool for this worker
    // uint32_t max_pending_kv_request;   // Maximum number of enqueued requests 

    // // thread unsafe queue, but we do not care the unsafety in single thread program
    // TAILQ_HEAD(, kv_request_internal) submit_queue;
    // TAILQ_HEAD(, kv_request_internal) resubmit_queue;

    //User thread get a free kv_req and enqueue it to req_used_ring.
    //worker thread get a used kv_req and enqueu a free req to req_free_ring.
    // 用户请求发向的环形队列
    struct spdk_ring *req_used_ring;  
    struct spdk_ring *req_free_ring;
    
    // struct spdk_poller *business_poller;
    // struct spdk_poller* slab_evaluation_poller;
    // struct spdk_poller* reclaim_poller;
    // struct spdk_poller* io_poller;

    struct object_cache_pool *kv_request_internal_pool;

    // struct pagechunk_mgr *pmgr;
    // struct reclaim_mgr   *rmgr;
    struct iomgr         *imgr;

    // struct spdk_thread* thread;
};

enum op_code { GET=0, PUT, DELETE, FIRST, SEEK, NEXT,};

struct process_ctx{
    struct worker_context *wctx;
    struct index_entry* entry;
    
    //a cached slab pointer to prevent the repetive computing for slab.
    struct slab* slab;
    //To record the old slab in case of the change of an item size.
    struct slab* old_slab;
    
    //When an item is updated not in place, this field will be used to record
    //the newly allocated entry infomation.
    struct index_entry new_entry;

    //For resubmit request, it is unnecessary to re-lookup in the mem index.
    uint32_t no_lookup;
};

struct kv_request_internal{
    TAILQ_ENTRY(kv_request_internal) link;

    uint32_t op_code;
    uint32_t shard;
    struct kv_item* item;
    worker_cb cb_fn;

    scan_cb scan_cb_fn;
    uint32_t scan_batch;

    void* ctx;

    struct process_ctx pctx;
};

struct kvs 
{
    /* data */
    char *kvs_name;
    uint32_t max_key_length;
    //uint32_t max_cache_chunks;


    uint32_t max_request_queue_size_per_worker;
    uint32_t max_io_pending_queue_size_per_worker;

    uint32_t nb_shards;
    struct slab_shard *shards;// ?

    struct pagecache *pagecache;

    //For any operation for blob, it shall be send to meta thread
    struct spdk_blob *super_blob;
    struct spdk_blob_store *bs_target;
    struct spdk_io_channel *meta_channel;
    struct spdk_thread *meta_thread;

    struct chunkmgr_worker_context *chunkmgr_worker;
    uint32_t nb_workers;
    struct worker_context **workers;
};
