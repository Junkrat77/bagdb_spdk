#ifndef OPTIONS_H
#define OPTIONS_H

#define PATH "/data3/sfy_data/worker%d"

/* Queue depth management */
#define PINNING 1
#define QUEUE_DEPTH 16
#define MAX_NB_PENDING_CALLBACKS_PER_WORKER (4*QUEUE_DEPTH)
#define NEVER_EXCEED_QUEUE_DEPTH 1 // Never submit more than QUEUE_DEPTH IO requests simultaneously, otherwise up to 2*MAX_NB_PENDING_CALLBACKS_PER_WORKER (very unlikely)
#define WAIT_A_BIT_FOR_MORE_IOS 0 // If we realize we don't have QUEUE_DEPTH IO pending when submitting IOs, check again if new incoming requests have arrived. Boost performance a tiny bit for zipfian workloads on AWS, but really not worthwhile

#define PAGE_SIZE (4*1024)
#define PAGE_CACHE_SIZE 512
namespace bagdb {

struct Options {
    Options()=default;
    int nb_workers = 1;
    int pagecache_level = 2;
  
//    size_t pagecache_size = (size_t)400 * 1024 * 1024;
  size_t pagecache_size = (size_t)PAGE_CACHE_SIZE * 1024 * 1024;
};

struct ReadOptions {
    ReadOptions()=default;
    bool scan=false;
    int num = 1;
};

struct WriteOptions {
    WriteOptions()=default;
    bool update=false;
};

}

#endif
