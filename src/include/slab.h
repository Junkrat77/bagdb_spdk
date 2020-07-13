#ifndef KVS_SLAB_H
#define KVS_SLAB_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
// #include "rbtree_uint.h"
// #include "io.h"

#include "spdk/blob.h"


struct slab_layout {
    //uint32_t slab_size;
    uint64_t blob_id;
    uint64_t resv;// blob addr
}

#define DEFAULT_KVS_PIN 0x1022199405121993u

void slab_get_slab_conf(uint32_t **slab_size_array, uint32_t *nb_slabs, uint32_t *chunk_pages)

struct super_layout{
    //If the kvs_pin is not equal to DEFAULT_KVS_PIN, it will be a invalid.
    uint64_t kvs_pin;
    uint32_t nb_shards;
    uint32_t nb_blob_per_shard;
    // uint32_t nb_slabs_per_shard;
    // uint32_t nb_chunks_per_reclaim_node;
    // uint32_t nb_pages_per_chunk;
    uint32_t max_key_length;
    struct slab_layout slab[0];
};