void worker_enqueue_get(struct worker_context* wctx,uint32_t shard,struct kv_item *item, worker_cb cb_fn, void* ctx);
void worker_enqueue_put(struct worker_context* wctx,uint32_t shard,struct kv_item *item, worker_cb cb_fn, void* ctx);
void worker_enqueue_delete(struct worker_context* wctx,uint32_t shard,struct kv_item *item, worker_cb cb_fn, void* ctx);