#ifndef _LINKED_LIST_INCLUDE_H
#define _LINKED_LIST_INCLUDE_H

#include <semaphore.h>
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include "log.h"

struct lnode;

typedef struct lnode {
		struct lnode* next;
		size_t value;
} lnode;

typedef struct {
		lnode* head;
		llog* log;
		/* The deleter holds both no_searcher and no_inserter while it's active */
		sem_t no_searcher; 
		/* Acts as a mutex so that only one inserter can be active at a time */
		sem_t no_inserter;
		pthread_mutex_t searcher_mutex;
		atomic_int searcher_count;
} llist;


llist *llist_new(size_t log_len);
void llist_free(llist*);
void llist_print(llist *list);
void llist_push_back(llist *list, size_t value);
int llist_delete(llist *list, size_t value);
lnode* llist_find(llist *list, size_t value);

lnode *lnode_new(size_t value);
void lnode_free(lnode *node);

#endif
