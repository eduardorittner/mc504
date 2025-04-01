#ifndef _LOG_INCLUDE_H
#define _LOG_INCLUDE_H

#include <stddef.h>
#include <stdatomic.h>
#include <stdlib.h>

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
  size_t cap;
  atomic_size_t len;
  wevent* events;
} llog;

llog* llog_new(size_t cap);
void llog_free(llog* log);
void llog_push(llog* log, wevent event);
void llog_print(llog* log);
void llog_print_pretty(llog* log);

#endif
