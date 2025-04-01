#include "log.h"
#include <assert.h>
#include <stdio.h>

llog *llog_new(size_t cap) {
  llog *log = (llog *)calloc(1, sizeof(*log));
  log->events = calloc(cap, sizeof(*log->events));
  log->cap = cap;
  log->len = 0;
  return log;
}

void llog_free(llog *log) {
  free(log->events);
  free(log);
}

void llog_push(llog *log, wevent event) {
  size_t tail = atomic_fetch_add(&log->len, 1);
  log->events[tail] = event;
}

char event_strings[12][32] = {
    "Searcher waiting", "Searcher entered",      "Searcher entered (first)",
    "Searcher leave",   "Searcher leave (last)", "Inserter waiting",
    "Inserter entered", "Inserter leave",        "Deleter waiting",
    "Deleter entered",  "Deleter leave"};

void llog_print(llog *log) {
  for (size_t i = 0; i < log->len; i++) {
    printf("%s\n", event_strings[log->events[i]]);
  }
}

void llog_print_pretty(llog *log) {
  int searchers_wait = 0, searchers_active = 0;
  int inserters_wait = 0, inserters_active = 0;
  int deleters_wait = 0, deleters_active = 0;

  for (size_t i = 0; i < log->len; i++) {
    switch (log->events[i]) {
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
