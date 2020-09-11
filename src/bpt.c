#include "bpt.h"


int map_internal_node(struct pagecache *page_cache_, internal_node_t **block, uint64_t index)const
{
    if(page_cache_->map_internal_node_page(page_cache_, index, (void **)block)){
        return 1;
    }
    int res = map(internal_node_fp, (void **)block, index);
    // TODO:这一部分需要改成异步操作
    return res;
}

int map_leaf_node(bpt *t, struct leaf2_node_t **block, )


uint64_t search_leaf(uint64_t index, const key_t &key) const;
uint64_t search_leaf(const key_t &key) const{
    return search_leaf(search_index(key), key);
}

int get_leaf(struct leaf2_node_t **block, struct pagecache *t, uint64_t index) const {
    int res = 0;
    
    // get the internode
    uint64_t org = 


    struct lru *lru;
    if(get_page(t->page_cache_, ))
    return res;
}


struct index_entry *bpt_search(bpt *t, key_t key, ) {
    leaf_node_t *leaf;
    // get the internode index
    meta_t *meta = t->meta;
    uint64_t org = meta->offset;
    struct pagecache *page_cache_ = t->page;
    int height = meta->height;
    while (height > 1) {
        struct internal_node_t *internode;
        // get page
        map_internal_node(t->page_cache_, &internode, org);// 该函数需要修改，暂且不考虑内存中不存在对应中间节点的情况
        // index_t *i = upper_bound(begin(node), end(node) - 1, key);
        // 判断key是否在当前节点中
        index_t *begin = &internode->children[0];
        for(int i = 0; i < n; i++) {
            if (begin->key != key) begin++;
            else break;
        }
        org = begin->child;
        --height;
    }

    // search leaf index
    uint64_t leaf_index;
    {
        struct internal_node_t *node;
        map_internal_node(t, &node, org);
        index_t *begin = &node->children[0];
        for(int i = 0; i < n; i++) {
            if (begin->key != key) begin++;
            else break;
        }
        leaf_index = begin->child;
    }

    return NULL;
}