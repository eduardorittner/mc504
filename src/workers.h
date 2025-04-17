#ifndef _WORKERS_INCLUDE_H
#define _WORKERS_INCLUDE_H

#include "linked-list.h"

// #define NDEBUG

/*
All the operations take the same context since none of them take a position
as argument:

- Search: Searches for the first ocurrence of value
- Insert: Inserts value at the end of the list
- Delete: Deletes the first ocurrence of value
*/

typedef struct {
  llist *list;
  size_t value;
} llist_ctx;

void* searcher_thread(void*);
void* inserter_thread(void*);
void* deleter_thread(void*);

int llist_searcher_acquire(llist*);
int llist_searcher_release(llist*);
int llist_inserter_acquire(llist*);
int llist_inserter_release(llist*);
int llist_deleter_acquire(llist*);
int llist_deleter_release(llist*);

#endif
