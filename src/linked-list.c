#include "linked-list.h"
#include "sync.h"
#include <stdio.h>
#include <stdlib.h>

llist *llist_new(void) {
  llist *list = calloc(1, sizeof(*list));
  pthread_mutex_init(&list->inserter, NULL);
  pthread_mutex_init(&list->searcher_mutex, NULL);
  sem_init(&list->no_searcher, 0, 1);
  sem_init(&list->no_inserter, 0, 1);
  list->searcher_count = 0;
  list->head = NULL;

  return list;
}

void llist_free(llist *list) {
  lnode *prev, *cur;
  prev = list->head;

  while (prev != NULL) {
    cur = prev->next;
    free(prev);
    prev = cur;
  }

  pthread_mutex_destroy(&list->inserter);
  pthread_mutex_destroy(&list->searcher_mutex);
  pthread_mutex_destroy(&list->st.lock);
  sem_destroy(&list->no_searcher);
  sem_destroy(&list->no_inserter);
  free(list);
}

void llist_print(llist *list) {
  size_t len = 128;
  char *buffer = calloc(len, sizeof(*buffer));
  int index = 0;

  for (lnode *cur = list->head; cur != NULL; cur = cur->next) {
    index +=
        snprintf(&buffer[index], len - (size_t)index - 1, "%zu->", cur->value);

    if (index < 0) {
      fprintf(stderr, "Couldn't print list");
      free(buffer);
      return;
    }

    if ((size_t)index > len) {
      // TODO add some handling here
      fprintf(stderr, "List too big to print");
      free(buffer);
      return;
    }
  }

  printf("%s\n", buffer);
  free(buffer);
}

void llist_push_back(llist *list, size_t value) {
  lnode *new_node = lnode_new(value);

  lnode **cur = &list->head;

  while (*cur != NULL) {
    // *cur = head
    cur = &(*cur)->next;
  }

  // *cur = head->next
  (*cur) = new_node;
}

int llist_delete(llist *list, size_t value) {
  lnode **cur = &list->head;

  while ((*cur) != NULL) {
    if ((*cur)->value == value) {
      lnode *deleted = *cur;
      *cur = (*cur)->next;
      lnode_free(deleted);
      return 0;
    }
    cur = &(*cur)->next;
  }

  return -1;
}

lnode *llist_find(llist *list, size_t value) {
  lnode **cur = &list->head;

  while ((*cur) != NULL) {
    if ((*cur)->value == value) {
      return *cur;
    }
    cur = &(*cur)->next;
  }

  return NULL;
}

lnode *lnode_new(size_t value) {
  lnode *node = calloc(1, sizeof(*node));
  node->next = NULL;
  node->value = value;

  return node;
}

void lnode_free(lnode *node) { free(node); }

/* A searcher can only search if there is no deleter currently holding the list
 */
int llist_searcher_acquire(llist *list) {
  if (pthread_mutex_lock(&list->searcher_mutex) < 0)
    return -1;


  /* Only the first searcher locks the no_searcher semaphore */
  if (list->searcher_count == 1) {
    if (sem_wait(&list->no_searcher) < 0)
      return -1;
  }

  /* Only increment searcher_count after we've locked no_searcher */
  list->searcher_count++;

  /* Unlock the mutex so other searchers can enter */
  if (pthread_mutex_unlock(&list->searcher_mutex) < 0)
    return -1;

  return 0;
}

int llist_searcher_release(llist *list) {
  if (pthread_mutex_lock(&list->searcher_mutex) < 0)
    return -1;

  list->searcher_count--;

  /* Only the last searcher unlocks the no_searcher semaphore */
  if (list->searcher_count == 0) {
    if (sem_post(&list->no_searcher) < 0)
      return -1;
  }

  if (pthread_mutex_unlock(&list->searcher_mutex) < 0)
    return -1;

  return 0;
}

/* An inserter can only enter if there are no deleters and no other inserters
 * holding the list */
int llist_inserter_acquire(llist *list) {
  // Lock the inserter mutex
  if (pthread_mutex_lock(&list->inserter) < 0)
    return -1;

  /* Since the deleter holds the no_inserter semaphore while it's active, we can
  use it as a way to find out if there is a deleter active */
  if (sem_wait(&list->no_inserter) < 0)
    return -1;

  return 0;
}

/* Since we're unlocking no_inserter before the inserter mutex, a waiting
 * deleter will get priority over a waiting inserter TODO (should we do this?)
 */
int llist_inserter_release(llist *list) {
  // Signal that there are currently no inserters
  if (sem_post(&list->no_inserter) < 0)
    return -1;

  // Unlock the mutex so another inserter can lock it
  if (pthread_mutex_unlock(&list->inserter) < 0)
    return -1;

  return 0;
}

/* A deleter can only search if there are no searchers or inserters holding
 the list */
int llist_deleter_acquire(llist *list) {
  if (sem_wait(&list->no_searcher))
    return -1;
  if (sem_wait(&list->no_inserter) < 0)
    return -1;

  /* There's no need to keep the mutex locked while in the critical section
   * since searchers will wait on no_searcher anyway */
  if (pthread_mutex_unlock(&list->searcher_mutex) < 0)
    return -1;
  return 0;
}

/* Unlocks the no_searcher and no_inserter semaphores */
int llist_deleter_release(llist *list) {
  if (sem_post(&list->no_inserter) < 0)
    return -1;
  if (sem_post(&list->no_searcher) < 0)
    return -1;
  return 0;
}

/* TODO: What can we return?
- Value: Redundant information since the caller already provides the value
- Int: 0 for found, -1 not found, gives little information
- Pointer to value: Good information, but can be invalidated by deleting it from
the list which can risk use-after-free or double-free
*/
void *searcher_thread(void *args) {
  llist_ctx ctx = *(llist_ctx *)args;

  llist_searcher_acquire(ctx.list);

  for (lnode *cur = ctx.list->head; cur != NULL; cur = cur->next) {
    if (cur->value == ctx.value) {
      int *result = malloc(sizeof(int));
      *result = 1;
      llist_searcher_release(ctx.list);
      return result;
    }
  }

  llist_searcher_release(ctx.list);
  return 0;
}

/* TODO: What can we return?
- Int: 0 for success, -1 on failure
- Enum: like int but clearer intent
- Pointer to value: Good but has the potential memory corruption problems
*/
void *inserter_thread(void *args) {
  llist_ctx ctx = *(llist_ctx *)args;

  llist_inserter_acquire(ctx.list);

  llist_push_back(ctx.list, ctx.value);

  llist_inserter_release(ctx.list);

  return NULL;
}

/* TODO: What can we return?
 */
void *deleter_thread(void *args) {
  llist_ctx ctx = *(llist_ctx *)args;

  llist_deleter_acquire(ctx.list);

  // TODO what happens when we can't delete?
  llist_delete(ctx.list, ctx.value);

  llist_deleter_release(ctx.list);

  return NULL;
}
