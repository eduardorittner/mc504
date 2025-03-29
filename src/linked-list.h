#ifndef _LINKED_LIST_INCLUDE_H
#define _LINKED_LIST_INCLUDE_H

#include <semaphore.h>
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>

struct lnode;

typedef struct lnode {
		struct lnode* next;
		size_t value;
} lnode;

typedef struct {
		lnode* head;
		/* There can only be one inserter at a time */
		pthread_mutex_t inserter; 
		/* Used by the deleter to exclusively access the list, must be signaled by the
		last searcher and inserter to leave */
		sem_t no_searcher; 
		sem_t no_inserter;
		pthread_mutex_t searcher_mutex;
		atomic_int searcher_count;
} llist;


llist *llist_new(void);
void llist_free(llist*);
void llist_print(llist *list);
void llist_push_back(llist *list, size_t value);
int llist_delete(llist *list, size_t value);
lnode* llist_find(llist *list, size_t value);

lnode *lnode_new(size_t value);
void lnode_free(lnode *node);

void* searcher_thread(void*);
void* inserter_thread(void*);
void* deleter_thread(void*);


#endif
