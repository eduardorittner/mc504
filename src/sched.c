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

void worker_queue_append_random(worker_queue *q) {
  q->ctxs[q->len].value = (size_t)rand();
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

llist *llist_random(size_t size, size_t log_len) {
  llist *list = llist_new(log_len);
  for (size_t i = 0; i < size; i++) {
    llist_push_back(list, (size_t)rand());
  }

  return list;
}

run_cfg run_cfg_new(size_t init, size_t s, size_t i, size_t d) {
  llist *list = llist_random(init, (s + i + d) * 6);
  run_cfg cfg = {0};
  cfg.initial_size = init;
  cfg.list = list;
  cfg.searchers = worker_queue_new(s, list, searcher_thread);
  cfg.inserters = worker_queue_new(i, list, inserter_thread);
  cfg.deleters = worker_queue_new(d, list, deleter_thread);
  return cfg;
}

void run_cfg_run(run_cfg *cfg) {
  int choice;
  while (cfg->searchers.len < cfg->searchers.cap ||
         cfg->inserters.len < cfg->inserters.cap ||
         cfg->deleters.len < cfg->deleters.cap) {

  random_choice:
    choice = rand() % 3;
    switch (choice) {
    case 0:
      if (cfg->searchers.len < cfg->searchers.cap) {
        worker_queue_append_random(&cfg->searchers);
        break;
      }
    case 1:
      if (cfg->inserters.len < cfg->inserters.cap) {
        worker_queue_append_random(&cfg->inserters);
        break;
      }
    case 2:
      if (cfg->deleters.len < cfg->deleters.cap) {
        worker_queue_append_random(&cfg->deleters);
        break;
      }
      goto random_choice;
    }
  }

  worker_queue_join(cfg->searchers);
  worker_queue_join(cfg->inserters);
  worker_queue_join(cfg->deleters);

  llog_print_pretty(cfg->list->log);

  llist_free(cfg->list);
}
