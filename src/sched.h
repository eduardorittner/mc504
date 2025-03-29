#include "workers.h"
#include "linked-list.h"
#include <stdlib.h>

typedef void*(*thread_fn)(void*);

/* Buffer that holds all the workers of a given type */
typedef struct {
  size_t cap;
  size_t len;
  llist* list;
  pthread_t *threads;
  llist_ctx *ctxs;
  thread_fn function;
} worker_queue;

worker_queue worker_queue_new(size_t cap, llist*, thread_fn f);

/* Configuration for a run, contains the number of searcher, inserters and
deleters which sould be created, as well as the initial list size */
typedef struct {
  size_t initial_size;
  llist *list;
  worker_queue searchers;
  worker_queue inserters;
  worker_queue deleters;
} run_cfg;

run_cfg run_cfg_new(size_t init, size_t s, size_t i, size_t d);
void run_cfg_run(run_cfg* cfg);
