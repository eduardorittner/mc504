#ifndef _SYNC_INCLUDE_H
#define _SYNC_INCLUDE_H

#include "linked-list.h"

/* Synchronization functions used by each of the worker types to access the
linked list */

int llist_searcher_acquire(llist*);
int llist_searcher_release(llist*);

int llist_inserter_acquire(llist*);
int llist_inserter_release(llist*);

int llist_deleter_acquire(llist*);
int llist_deleter_release(llist*);

#endif
