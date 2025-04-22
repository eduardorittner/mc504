#include "linked-list.h"
#include "sched.h"
#include "workers.h"
#include <stdio.h>
#include <stdlib.h>

worker_queue worker_queue_new(size_t cap, llist *list, thread_fn f) {
  worker_queue q = {0};
  q.cap = cap;
  q.len = 0;
  q.function = f;
  // TODO really?
  q.threads = (pthread_t *)calloc(cap, sizeof(pthread_t));
  q.ctxs = (llist_ctx *)calloc(cap, sizeof(llist_ctx));

  for (size_t i = 0; i < cap; i++) {
    q.ctxs[i].list = list;
  }

  return q;
}

void worker_queue_append_random(worker_queue *q, size_t random_upper_bound) {
  q->ctxs[q->len].value = (size_t)(1 + (size_t)rand() % random_upper_bound);
  pthread_create(&q->threads[q->len], NULL, q->function, &q->ctxs[q->len]);
  q->len++;
}

void worker_queue_free(worker_queue q) {
  free(q.threads);
  free(q.ctxs);
}

void worker_queue_join(worker_queue q) {
  for (size_t i = 0; i < q.len; i++) {
    pthread_join(q.threads[i], NULL);
  }
  worker_queue_free(q);
}

llist *llist_random(size_t size, size_t random_upper_bound) {
  llist *list = llist_new();
  for (size_t i = 0; i < size; i++) {
    llist_push_back(list, (size_t)(1 + (size_t)rand() % random_upper_bound));
  }

  return list;
}

run_cfg run_cfg_new(size_t init, size_t s, size_t i, size_t d, size_t random_upper_bound) {
  llist *list = llist_random(init, random_upper_bound);
  run_cfg cfg = {0};
  cfg.initial_size = init;
  cfg.list = list;
  cfg.searchers = worker_queue_new(s, list, searcher_thread);
  cfg.inserters = worker_queue_new(i, list, inserter_thread);
  cfg.deleters = worker_queue_new(d, list, deleter_thread);
  return cfg;
}

void run_cfg_run(run_cfg *cfg, size_t random_upper_bound) {
  int choice;
  while (cfg->searchers.len < cfg->searchers.cap ||
        cfg->inserters.len < cfg->inserters.cap ||
        cfg->deleters.len < cfg->deleters.cap) {

  random_choice:
    choice = rand() % 3;
    switch (choice) {
    case 0:
      if (cfg->searchers.len < cfg->searchers.cap) {
        worker_queue_append_random(&cfg->searchers, random_upper_bound);
        break;
      }
    case 1:
      if (cfg->inserters.len < cfg->inserters.cap) {
        worker_queue_append_random(&cfg->inserters, random_upper_bound);
        break;
      }
    case 2:
      if (cfg->deleters.len < cfg->deleters.cap) {
        worker_queue_append_random(&cfg->deleters, random_upper_bound);
        break;
      }
      goto random_choice;
    }
  }

  worker_queue_join(cfg->searchers);
  worker_queue_join(cfg->inserters);
  worker_queue_join(cfg->deleters);

  llist_free(cfg->list);
}
