#ifndef _LOG_INCLUDE_H
#define _LOG_INCLUDE_H

#include <stddef.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

//#define LOG

typedef enum {
  SEARCHER_WAIT,
  SEARCHER_ENTER,
  SEARCHER_ENTER_FIRST,
  SEARCHER_LEAVE,
  SEARCHER_LEAVE_LAST,
  INSERTER_WAIT,
  INSERTER_ENTER,
  INSERTER_LEAVE,
  DELETER_WAIT,
  DELETER_ENTER,
  DELETER_LEAVE,
} wevent;

typedef struct {
  wevent type;
  struct timespec time;
  pthread_t id;
} llog_entry; 

typedef struct {
  size_t cap;
  atomic_size_t len;
  llog_entry* log;
} llog;

llog* llog_new(size_t cap);
void llog_free(llog* log);
void llog_push(llog* log, wevent event);
void llog_print(llog* log);
void llog_print_pretty(llog* log);
void llog_print_stats(llog* log);

#endif
