#ifndef _LINKED_LIST_INCLUDE_H
#define _LINKED_LIST_INCLUDE_H

#include <semaphore.h>
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include "int-list.h"

struct lnode;

typedef struct lnode {
	struct lnode* next;
	size_t value;
} lnode;

/*
Only used for debugging purposes, always updated whenever a thread does any
action.

searchers is a list of integers such that each integer is a value that the thread is processing.
inserters is an unique integer which is either the value being inserted by the thread or NULL in case no inserter is running.
deleters is an unique integer which is either the value being deleted by the thread or NULL in case no deleters is running.

The counters for deleters and inserters is just for a debugging purpose to ensure that just one is running at a time.
*/
typedef struct {
	int_list searchers;
	atomic_int searchers_waiting;
	size_t inserters;
	atomic_int inserters_waiting;
	size_t deleters;
	atomic_int deleters_waiting;
	pthread_mutex_t lock;
} state;

typedef struct {
	lnode* head;
	/* The deleter holds both no_searcher and no_inserter while it's active */
	sem_t no_searcher; 
	/* Acts as a mutex so that only one inserter can be active at a time */
	sem_t no_inserter;
	pthread_mutex_t searcher_mutex;
	atomic_int searcher_count;
	state st;
} llist;


llist *llist_new(void);
void llist_free(llist*);
void llist_print(llist *list);
void llist_push_back(llist *list, size_t value);
int llist_delete(llist *list, size_t value);
lnode* llist_find(llist *list, size_t value);

lnode *lnode_new(size_t value);
void lnode_free(lnode *node);

#endif
