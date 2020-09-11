#include "pagecache.h"
/*
 * Get a page from the page cache.
 * *page will be set to the address in the page cache
 * @return 1 if the page already contains the right data, 0 otherwise.
 */
int get_page(struct pagecache * ,uint64_t hash, uint32_t level, void **page, struct lru **lru) {
   void *dst;
   struct lru *lru_entry;
   pagecache_entry_t tmp_entry;
   void * tmp_page;
   uint64_t old_hash = 0;

   // Is the page already cached?
   pagecache_entry_t *e = tree_lookup(Level[level].hash_to_page, hash);
   if(e) {
      dst = e->page;
      lru_entry = (struct lru *)e->lru;
      assert(lru_entry->hash == hash);
      bump_page_in_lru(level, lru_entry, hash);
      *page = dst;
      *lru = lru_entry;
      return 1;
   }

   // Otherwise allocate a new page, either a free one, or reuse the oldest
   struct Level * plevel = &Level[level];
   
   if(true == find_in_freelist(&tmp_page))
   {
        dst = tmp_page;
        lru_entry = add_page_in_lru(tmp_page, level, hash);
//     bump_page_in_lru(p, level, lru_entry,hash);
   }
   else if(used_page_num  < page_num) {
      dst = &cached_data[PAGE_SIZE*used_page_num];
      lru_entry = add_page_in_lru(dst, level, hash);
      used_page_num += 1;
   } 
   else if(true == find_in_lower_level(level, &tmp_page))
   {
      dst = tmp_page;
      lru_entry = add_page_in_lru(tmp_page, level, hash);
 //     bump_page_in_lru(p,level,lru_entry, hash);
   }
   else
   {
      lru_entry = plevel->oldest_page;
      dst = plevel->oldest_page->page;
      old_hash = plevel->oldest_page->hash;
      tree_delete(plevel->hash_to_page, plevel->oldest_page->hash, &old_entry);

      lru_entry->hash = hash;
      lru_entry->page = dst;
      bump_page_in_lru(level, lru_entry, hash);
   }

   // Remember that the page cache now stores this hash
   tree_insert(plevel->hash_to_page, hash, old_entry, dst, lru_entry);

   lru_entry->contains_data = 0;
   lru_entry->dirty = 0; // should already be equal to 0, but we never know
   *page = dst;
   *lru = lru_entry;

   return 0;
}


int map_internal_node_page(struct pagecache *page_cache_ , uint64_t hash, void **page) 
{
   void * dst = NULL, *tmp_page = NULL;
   void * ptr = internel_node_get(page_cache_, hash);

   if(NULL != ptr){
      *page = ptr;
      return 1; // 返回空page供其从内存读取
   }

   if(true == find_in_freelist(&tmp_page)){
      dst = tmp_page;   //
   }
   else if(page_cache_->used_pages_num  < page_cache_->page_num) {// 从cache_date中分配
      dst = &cached_data[PAGE_SIZE * page_cache_->used_page_num];
      page_cache_->used_page_num += 1;
   } 
   else if(true == find_in_lower_level(0, &tmp_page)){
      dst = tmp_page;
   }
   else{
      assert(0);
   }

   internel_node_insert(hash, dst);//插入中间节点
   *page = dst;
   return 0;
}


// get internode from pagecache 
void *internel_node_get(struct pagecache *page_cache_, uint64_t hash) {
   uint64_t list_index = hash / INTERNEL_NODE_LIST_ITEMS;
   uint64_t item_index = hash % INTERNEL_NODE_LIST_ITEMS;
   assert(MAX_INTERNEL_NODE_LIST > list_index);
   void **tmp;
   tmp = page_cache_->internel_node[list_index];
   if (NULL == tmp) {
      page_cache_->internel_node[list_index] = (void **)malloc(sizeof(void *) * INTERNEL_NODE_LIST_ITEMS);
      assert(NULL != page_cache_->internel_node[list_index]);
      tmp = page_cache_->internel_node[list_index];
      for (int i = 0; i < INTERNEL_NODE_LIST_ITEMS; i++) {
         tmp[i] = NULL;
      }
   }

   return temp[item_index];
}


// 
bool find_in_freelist(struct pagecache *page_cache_, void **page) {
   if(page_cache_->freelistnum == 0) return false;

   *page = page_cache_->freelisthead;
   page_cache_->freelistnum --;
   if (page_cache_->freelistnum > 0) {
      page_cache_->freelisthead = *(void **)*page;
   }
   return true;
}

bool find_in_lower_level(struct pagecache page_cache_, uint32_t level, void **page) {
   if(level  >= page_cache_->nlevel)
        return false;
   void *dst = NULL;
   struct lru *lru_entry; 
   struct Level *Level = page_cache_->Level;
   pagecache_entry_t *old_entry = NULL;
   struct Level * Level_entry = &Level[level];

   if((level + 1 < page_cache->nlevel )
      && find_in_lower_level(level + 1,  page)){
      return true;
   }
   if(Level_entry->usedlru <= Level_entry->min_pages)
      return false;

   lru_entry = Level_entry->oldest_page;
   *page = Level_entry->oldest_page->page;
   
   // 淘汰刷盘
   if(lru_entry->dirty == 1){
      // flus data to do 
   }
   
   tree_delete(Level_entry->hash_to_page, Level_entry->oldest_page->hash, &old_entry);
   Level_entry->oldest_page = Level_entry->oldest_page->prev;
   if(Level_entry->oldest_page != NULL)
       Level_entry->oldest_page->next = NULL;
   
   Level_entry->usedlru --;
   return true;
}

void internel_node_insert(struct pagecache page_cache_, uint64_t hash, void *page)
{
   uint64_t list_index = hash / INTERNEL_NODE_LIST_ITEMS;
   uint64_t item_index = hash % INTERNEL_NODE_LIST_ITEMS;
   assert(MAX_INTERNEL_NODE_LIST > list_index);
   void** tmp;
   tmp = page_cache->internel_node[list_index];
   if(NULL == internel_node[list_index])
   {
      internel_node[list_index] = (void **)malloc(sizeof(void *) * INTERNEL_NODE_LIST_ITEMS);
      assert(NULL != internel_node[list_index]);
      tmp = internel_node[list_index];
      for(int i = 0; i < INTERNEL_NODE_LIST_ITEMS; i++){
         tmp[i] = NULL;
      }
   }
	
   tmp[item_index] = page;
}

}

