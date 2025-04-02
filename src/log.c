#include "log.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>

/* All public llog functions return immediately if LOG is not defined */

llog *llog_new(size_t cap) {
#ifndef LOG
  return NULL;
#endif
  llog *log = (llog *)calloc(1, sizeof(*log));
  log->log = calloc(cap, sizeof(*log->log));
  log->cap = cap;
  log->len = 0;
  return log;
}

void llog_free(llog *log) {
#ifndef LOG
  return;
#endif
  free(log->log);
  free(log);
}

void llog_push(llog *log, wevent event) {
#ifndef LOG
  return;
#endif
  size_t tail = atomic_fetch_add(&log->len, 1);
  struct timespec time;
  clock_gettime(CLOCK_MONOTONIC, &time);
  pthread_t id = pthread_self();

  log->log[tail] = (llog_entry){.time = time, .type = event, .id = id};
}

char event_strings[12][32] = {
    "Searcher waiting", "Searcher entered",      "Searcher entered (first)",
    "Searcher leave",   "Searcher leave (last)", "Inserter waiting",
    "Inserter entered", "Inserter leave",        "Deleter waiting",
    "Deleter entered",  "Deleter leave"};

void llog_print(llog *log) {
#ifndef LOG
  return;
#endif
  for (size_t i = 0; i < log->len; i++) {
    printf("%s\n", event_strings[log->log[i].type]);
  }
}

void llog_print_pretty(llog *log) {
#ifndef LOG
  return;
#endif
  int searchers_wait = 0, searchers_active = 0;
  int inserters_wait = 0, inserters_active = 0;
  int deleters_wait = 0, deleters_active = 0;

  for (size_t i = 0; i < log->len; i++) {
    switch (log->log[i].type) {
    case SEARCHER_WAIT:
      searchers_wait++;
      break;
    case SEARCHER_ENTER:
      searchers_wait--;
      searchers_active++;
      assert(deleters_active == 0);
      assert(searchers_active > 1);
      break;
    case SEARCHER_ENTER_FIRST:
      printf("First searcher\n");
      searchers_wait--;
      searchers_active++;
      assert(deleters_active == 0);
      assert(searchers_active == 1);
      break;
    case SEARCHER_LEAVE:
      searchers_active--;
      assert(searchers_active > 0);
      assert(deleters_active == 0);
      break;
    case SEARCHER_LEAVE_LAST:
      printf("Last searcher\n");
      searchers_active--;
      assert(deleters_active == 0);
      assert(searchers_active == 0);
      break;
    case INSERTER_WAIT:
      inserters_wait++;
      break;
    case INSERTER_ENTER:
      printf("Inserter entered\n");
      inserters_wait--;
      inserters_active++;
      assert(inserters_active == 1);
      assert(deleters_active == 0);
      break;
    case INSERTER_LEAVE:
      printf("Inserter left\n");
      inserters_active--;
      assert(inserters_active == 0);
      assert(deleters_active == 0);
      break;
    case DELETER_WAIT:
      deleters_wait++;
      break;
    case DELETER_ENTER:
      printf("Deleter entered\n");
      deleters_wait--;
      deleters_active++;
      assert(inserters_active == 0);
      assert(deleters_active == 1);
      break;
    case DELETER_LEAVE:
      printf("Deleter left\n");
      deleters_active--;
      assert(inserters_active == 0);
      assert(deleters_active == 0);
      break;
    }
  }
}

typedef struct {
  enum { worker_searcher, worker_inserter, worker_deleter } worker_type;
  struct timespec wait;
  struct timespec enter;
  struct timespec leave;
  pthread_t id;
} worker;

void store_timestamp(worker *w, llog_entry entry) {
  if (w->id != 0)
    assert(w->id == entry.id);

  w->id = entry.id;
  switch (entry.type) {
  case SEARCHER_WAIT:
    w->worker_type = worker_searcher;
    w->wait = entry.time;
    break;
  case SEARCHER_ENTER:
    w->worker_type = worker_searcher;
    w->enter = entry.time;
    break;
  case SEARCHER_ENTER_FIRST:
    w->worker_type = worker_searcher;
    w->enter = entry.time;
    break;
  case SEARCHER_LEAVE:
    w->worker_type = worker_searcher;
    w->leave = entry.time;
    break;
  case SEARCHER_LEAVE_LAST:
    w->worker_type = worker_searcher;
    w->leave = entry.time;
    break;
  case INSERTER_WAIT:
    w->worker_type = worker_inserter;
    w->wait = entry.time;
    break;
  case INSERTER_ENTER:
    w->worker_type = worker_inserter;
    w->enter = entry.time;
    break;
  case INSERTER_LEAVE:
    w->worker_type = worker_inserter;
    w->leave = entry.time;
    break;
  case DELETER_WAIT:
    w->worker_type = worker_deleter;
    w->wait = entry.time;
    break;
  case DELETER_ENTER:
    w->worker_type = worker_deleter;
    w->enter = entry.time;
    break;
  case DELETER_LEAVE:
    w->worker_type = worker_deleter;
    w->leave = entry.time;
    break;
  }
}

size_t worker_push(worker *workers, size_t len, llog_entry entry) {
  for (size_t i = 0; i < len; i++) {
    if (workers[i].id == entry.id) {
      store_timestamp(&workers[i], entry);
      return len;
    }
  }

  store_timestamp(&workers[len], entry);
  return len + 1;
}

void llog_print_stats(llog *log) {
#ifndef LOG
  return;
#endif
  worker *workers = calloc(log->len * 3, sizeof(*workers));
  size_t len = 0;
  for (size_t i = 0; i < log->len; i++) {
    len = worker_push(workers, len, log->log[i]);
  }

  long searcher_n = 0;
  long inserter_n = 0;
  long deleter_n = 0;

  long searcher_wait_avg = 0;
  long inserter_wait_avg = 0;
  long deleter_wait_avg = 0;

  long searcher_in_avg = 0;
  long inserter_in_avg = 0;
  long deleter_in_avg = 0;

  long wait_time, in_time;

  char worker_names[3][16] = {"Searcher", "Inserter", "Deleter"};

  for (size_t i = 0; i < len; i++) {
    worker w = workers[i];
    wait_time = w.enter.tv_nsec - w.wait.tv_nsec;
    in_time = w.leave.tv_nsec - w.enter.tv_nsec;
    printf("%s waiting: %ld in: %ld\n", worker_names[w.worker_type], wait_time,
           in_time);

    switch (w.worker_type) {
    case worker_searcher:
      searcher_wait_avg += wait_time;
      searcher_in_avg += in_time;
      searcher_n++;
      break;
    case worker_inserter:
      inserter_wait_avg += wait_time;
      inserter_in_avg += in_time;
      inserter_n++;
      break;
    case worker_deleter:
      deleter_wait_avg += wait_time;
      deleter_in_avg += in_time;
      deleter_n++;
      break;
    }
  }
  searcher_wait_avg /= searcher_n;
  inserter_wait_avg /= inserter_n;
  deleter_wait_avg /= deleter_n;

  searcher_in_avg /= searcher_n;
  inserter_in_avg /= inserter_n;
  deleter_in_avg /= deleter_n;

  printf("searcher wait avg: %ld, in avg: %ld\n", searcher_wait_avg,
         searcher_in_avg);
  printf("inserter wait avg: %ld, in avg: %ld\n", inserter_wait_avg,
         inserter_in_avg);
  printf("deleter wait avg: %ld, in avg: %ld\n", deleter_wait_avg,
         deleter_in_avg);

  free(workers);
}
