#include <string.h>
#include <iostream>
#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <cstdlib>
#include "option.h"
#include "memory-item.h"


#define MAX_INTERNEL_NODE_LIST 512
#define INTERNEL_NODE_LIST_ITEMS 512
#define MIN_PAGE_NUM 1024
typedef struct index_entry pagecache_entry_t;

typedef btree_t* hash_t;

#define tree_create() btree_create()
#define tree_lookup(h, hash) ({ int res = btree_find((h), (unsigned char*)&(hash), sizeof(hash), &tmp_entry); res?&tmp_entry:NULL; })
#define tree_delete(h, hash, old_entry) \
   do { \
      btree_delete((h), (unsigned char*)&(hash), sizeof(hash)); \
   } while(0);
#define tree_insert(h, hash, old_entry, dst, lru_entry) \
   do { \
      pagecache_entry_t new_entry; \
      new_entry.page = dst, new_entry.lru = lru_entry ; \
      btree_insert((h),(unsigned char*)&(hash),sizeof(hash), &new_entry); \
   } while(0)
struct lru {
   struct lru *prev;
   struct lru *next;
   uint64_t hash;
   void *page;
   int contains_data;
   int dirty;
};

struct Level{
   hash_t hash_to_page;
   struct lru *oldest_page, *newest_page;
   uint32_t usedlru;
   uint32_t min_pages;
};

struct pagecache {
    char *cached_data;
    struct Level *Level;
    uint32_t nlevel;
    struct lru *used_pages;
    uint32_t page_num; 
    size_t used_page_num;
    void * freelisthead;
    uint32_t freelistnum;
    void**  internel_node[MAX_INTERNEL_NODE_LIST];
    static pagecache* instance;
}

int map_internal_node_page(struct pagecache *page_cache_ , uint64_t hash, void **page);