#ifndef KVS_H
#define KVS_H
#include <stdbool.h>
#include "index.h"
#include "item.h"
#include "worker.h"

#include "spdk/event.h"

struct kvs_start_opts{
    // nb_worker should be 1,2,4,8,16... and less than nb_shards
    char* kvs_name;
    uint32_t nb_works;
    uint32_t max_request_queue_size_per_worker;
    uint32_t max_io_pending_queue_size_per_worker;
    uint32_t max_cache_chunks;

    uint32_t reclaim_batch_size;
    uint32_t reclaim_percentage_threshold;
    char* devname;

    struct spdk_app_opts *spdk_opts;
    void (*startup_fn)(void*ctx, int kverrno);
    void* startup_ctx;
}

struct kvs_start_opts{
    // nb_worker should be 1,2,4,8,16... and less than nb_shards
    char* kvs_name;
    uint32_t nb_works;
    uint32_t max_request_queue_size_per_worker;
    uint32_t max_io_pending_queue_size_per_worker;
    uint32_t max_cache_chunks;

    uint32_t reclaim_batch_size;
    uint32_t reclaim_percentage_threshold;
    char* devname;

    struct spdk_app_opts *spdk_opts;
    void (*startup_fn)(void*ctx, int kverrno);
    void* startup_ctx;
};

//The kvs is implemented in sinleton mode. Only one instance is the kvs allowed to startup.
void kvs_start_loop(struct kvs_start_opts *opts);
void kvs_shutdown(void);
bool kvs_is_started(void);

// The whole item shall be filled
void kv_get_async(struct kv_item *item, kv_cb cb_fn, void* ctx);
void kv_put_async(struct kv_item *item, kv_cb cb_fn, void* ctx);