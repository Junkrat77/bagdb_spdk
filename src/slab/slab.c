#include "slab.h"
#include "pagechunk.h"
#include "kvutil.h"
#include "kverrno.h"

#include "spdk/thread.h"
#include "spdk/log.h"
#include "spdk/env.h"

//All slab sizes are 4-bytes-aligment.
static const uint32_t _g_slab_chunk_pages = 135;
// static uint32_t slab_sizes[]={
//     32, 36, 40,48, 56, 64, 72, 88, 96, 104, 124, 144, 196, 224, 256, 272, 292, 316, 344, 372, 408, 456,
//     512, 584, 684,
//     768,864,960,1024,1152,1280,1440,1536,1728,1920,2048,2304,2560,2880,3072,3456,3840,4096,4320,4608,5120,
//     5760,6144,6912,7680,8640,9216,10240
// };
static const uint32_t slab_size[] = {0, 1}; //0代表叶子节点， 1代表背包节点

void slab_get_slab_conf(uint32_t **slab_size_array, uint32_t *nb_slabs, uint32_t *chunk_pages){
    uint32_t _nb_slabs = sizeof(slab_sizes)/sizeof(slab_sizes[0]);
    *slab_size_array = slab_sizes;
    *nb_slabs = _nb_slabs;
    *chunk_pages = _g_slab_chunk_pages;// 一个chunk包括的page数？
}