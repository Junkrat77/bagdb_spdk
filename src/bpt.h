#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <vector>
#include "pagecache.h"
#define OFFSET_META 0
#define OFFSET_BLOCK OFFSET_META + sizeof(meta_t)
#define SIZE_NO_CHILDREN sizeof(internal_node_t) - BP_ORDER * sizeof(index_t)

/*index next block*/
#define FREE_LIST_NODE_MAX  125
#define key_t uint64_t

enum type{
    INTERNAL_NODE = 0,
    LEAF_NODE,
    BAG_NODE
};

struct bplus_index
{
        int fd;
        uint64_t next_new_id;
        uint64_t freelist[FREE_LIST_NODE_MAX];
        uint32_t freecount;
        uint32_t next_freeindex;
};

/* meta information of B+ tree */
typedef struct {
    size_t node_order; /* `order` of B+ tree */
    size_t key_order;  /* order of key*/
    size_t value_size; /* size of value */
    size_t key_size;   /* size of key */
    size_t internal_node_num; /* how many internal nodes */
    size_t leaf_node_num;     /* how many leafs */
    size_t bag_node_num;      /* how many bags */
    size_t height;            /* height of tree (exclude leafs) */
    // off_t slot;        /* where to store new block */
    struct bplus_index internal_node_id_mgr;
    struct bplus_index leaf_id_mgr;
    uint64_t root_offset; /* where is the root of internal nodes */
    uint64_t leaf_offset; /* where is the first leaf */
    uint16_t tmp;
} meta_t;

/* internal nodes' index segment */
typedef struct index_t {
    key_t key;
    uint64_t child; /* child's offset */
} index_t;

/***
 * internal node block
 ***/
struct internal_head_t{
    uint64_t parent; /* parent node offset */
    uint64_t next;
    uint64_t prev;
    size_t n; /* how many children */
};
struct internal_node_t {
	typedef index_t *child_t;
    uint64_t parent; /* parent node offset */
    uint64_t next;
    uint64_t prev;
    size_t n; /* how many children */
    index_t children[NODE_ORDER];
};

/* the final record of value */

/* leaf node block */

struct l_bagid_t
{
    uint64_t bagid;
    uint8_t  v_num;
    struct key_t bigest_key;
};


struct l_delbag_t
{
    uint8_t bagid_index;
    uint64_t bitmap;
};

struct lb_value_t
{
    uint16_t size;
    char data;

};

struct l_key_t
{
    struct key_t key;
    uint16_t offset;
};

typedef struct leaf2_node_t{

    uint32_t parent;
    uint32_t next, prev;
    uint16_t size;
    uint16_t key_num;
    uint16_t bagid_num;
    uint16_t value_num;
    uint16_t delbag_num;
    
    uint16_t bagid_index_head;
    uint16_t key_index_head;
    uint16_t value_insert;
    uint16_t delbag_head;

}leaf_node_t;


struct bag_node_t{
    uint16_t type; // oh no 
    uint16_t value_num;
    uint16_t index_num;
    uint16_t size;
    uint16_t index_head;
    uint16_t value_insert;
};

typedef struct bpt {
    char internal_node_path[512];
    char leaf_path[512];
    meta_t *meta;
    struct pagecache *page_cache_;

    /* multi-level file open/close */
    mutable int internal_node_fp, leaf_fp;
    mutable int fp_level;
} bpt;

struct index_entry *bpt_search(struct bpt *t, struct kv_item *item);

// struct l_key_t * get_key(const struct leaf2_node_t * leaf, const uint16_t index);
// void insert_key(struct leaf2_node_t * leaf, const key_t &key, const uint16_t offset);
// void update_key(struct leaf2_node_t * leaf, const key_t &key, const uint16_t offset);
// void delete_key(struct leaf2_node_t * leaf, struct l_key_t *key);

// template <class T>
// struct lb_value_t * get_value(T * node, const uint16_t offset);
// template <class T>
// uint16_t insert_value(T * node, const value_t* value);
// template <class T>
// uint16_t insert_value(T * node, const struct lb_value_t* value);
// template <class T>
// void delete_value(T * node, const uint16_t offset);

// struct l_bagid_t * get_bagid(struct leaf2_node_t * leaf, const uint16_t index);
// uint16_t insert_bagid(struct leaf2_node_t * leaf, const uint64_t bag_id);
// void delete_bagid(struct leaf2_node_t * leaf, const uint16_t index);
// uint16_t insert_del_bagid(struct leaf2_node_t * leaf, const uint16_t bagid_index,  const uint16_t value_index);
// uint16_t update_del_bagid(struct leaf2_node_t * leaf, uint8_t bagid, bag_node_t bag);
// struct l_delbag_t * get_delbag(leaf_node_t * leaf, const uint16_t n);
// uint16_t delete_delbag(leaf_node_t * leaf, const uint16_t bagindex);
// struct l_bagid_t* find_bagid(leaf_node_t * leaf, const key_t& key);
// bool value_is_in_bag(struct l_key_t * key);
