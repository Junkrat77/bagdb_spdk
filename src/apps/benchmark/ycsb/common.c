#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "common.h"
#include "item.h"
#include "kvutil.h"
#include "kvs.h"
#include "kverrno.h"

/*
 * Create a workload item for the database
 */
char *create_unique_item(size_t item_size, uint64_t uid) {
   char *item = malloc(item_size);
   struct item_metadata *meta = (struct item_metadata *)item;
   meta->key_size = 8;
   meta->value_size = item_size - 64 - sizeof(*meta);

   char *item_key = &item[sizeof(*meta)];
   char *item_value = &item[sizeof(*meta) + meta->key_size];
   *(uint64_t*)item_key = uid;
   *(uint64_t*)item_value = uid;
   return item;
}

// struct slab_callback *bench_cb(void) {
//    struct slab_callback *cb = malloc(sizeof(*cb));
//    cb->cb = compute_stats;
//    cb->payload = allocate_payload();
//    return cb;
// }

void  repopulate_db(struct workload *w) {
    return;
//    uint64_t nb_items = kvs_get_nb_items();
//    uint64_t nb_inserts = ( nb_items > w->nb_items_in_db)?0:(w->nb_items_in_db - nb_items);

//    if(nb_inserts != w->nb_items_in_db) { 
//       // Database at least partially populated
//       // Check that the items correspond to the workload
//       _check_db(w);
//    }

//    if(nb_inserts == 0) {
//       return;
//    }

//    // Say that this database is for that workload.
//    if(nb_items == 0) {
//       _add_db_flag(w);
//    } else {
//       nb_items--; // do not count the workload_item
//    }

//    if(nb_items != 0 && nb_items != w->nb_items_in_db) {
//       /*
//        * Because we shuffle elements, we don't really want to start with a small database and have all the higher order elements at the end, that would be cheating.
//        * Plus, we insert database items at random positions (see shuffle below) and I am too lazy to implement the logic of doing the shuffle minus existing elements.
//        */
//       printf("The database contains %lu elements but the benchmark is configured to use %lu. Please delete the DB first.\n", nb_items, w->nb_items_in_db);
//       exit(-1);
//    }

//    uint64_t *pos = NULL;

//    printf("Initializing big array to insert elements in random order... This might take a while. (Feel free to comment but then the database will be sorted and scans much faster -- unfair vs other systems)\n");
//    pos = malloc(w->nb_items_in_db * sizeof(*pos));
//    for(uint64_t i = 0; i < w->nb_items_in_db; i++)
//       pos[i] = i;

//    // To be fair to other systems, we shuffle items in the DB so that the DB is not fully sorted by luck
//    printf("Start shuffling\n");
//    kv_shuffle(pos, nb_inserts);
//    printf("Shuffle  completes\n");

//    pthread_t *threads = malloc(w->nb_load_injectors*sizeof(*threads));

//    for(int i = 0; i < w->nb_load_injectors; i++) {
//       struct rebuild_pdata *pdata = calloc(1,sizeof(*pdata));
//       pdata->id = w->start_core + i;
//       pdata->start = (w->nb_items_in_db / w->nb_load_injectors)*i;
//       pdata->end = (w->nb_items_in_db / w->nb_load_injectors)*(i+1);
//       if(i == w->nb_load_injectors - 1)
//          pdata->end = w->nb_items_in_db;
//       pdata->w = w;
//       pdata->pos = pos;
//       pthread_create(&threads[i], NULL, repopulate_db_worker, pdata);
//    }

//    //wait the finishing of db repopulating.
//    for(int i = 0; i < w->nb_load_injectors; i++){
//       pthread_join(threads[i], NULL);
//    }
//    free(threads);
//    free(pos);
}

static pthread_barrier_t barrier;
void* do_workload_thread(void *pdata) {
   struct thread_data *data = pdata;

   init_seed();
   pin_me_on(data->id);
   pthread_barrier_wait(&barrier);

   data->workload->api->launch(data->workload, data->benchmark, data->id);

   return NULL;
}


void run_workload(struct workload *w, bench_t b) {
    struct thread_data *pdata = malloc(w->nb_load_injectors*sizeof(*pdata));

    w->nb_requests_per_thread = w->nb_requests / w->nb_load_injectors;
    pthread_barrier_init(&barrier, NULL, w->nb_load_injectors);

    if(!w->api->handles(b)){
        printf("The database has not been configured to run this benchmark! (Are you trying to run a production benchmark on a database configured for YCSB?)"); 
        exit(-1);
        return;
    }

    pthread_t *threads = malloc(w->nb_load_injectors*sizeof(*threads));
    for(int i = 0; i < w->nb_load_injectors; i++) {
        pdata[i].id = w->start_core + i;
        pdata[i].workload = w;
        pdata[i].benchmark = b;
        pthread_create(&threads[i], NULL, do_workload_thread, &pdata[i]);
    }
    
    for(int i = 0; i < w->nb_load_injectors; i++){
        pthread_join(threads[i], NULL);
    }
        
    free(threads);
    free(pdata);
}