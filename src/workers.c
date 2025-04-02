#include "sync.h"
#include "workers.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* A searcher can only search if there is no deleter currently holding the list
 */
int llist_searcher_acquire(llist *list) {
#ifdef LOG
  llog_push(list->log, SEARCHER_WAIT);
#endif

  /* Lock the mutex to update searcher count */
  mutex_acquire(&list->searcher_mutex);

  list->searcher_count++;

  /* Only the first searcher locks the no_searcher semaphore */
  if (list->searcher_count == 1) {
    sem_acquire(&list->no_searcher);
#ifdef LOG
    llog_push(list->log, SEARCHER_ENTER_FIRST);
#endif
  } else {
#ifdef LOG
    llog_push(list->log, SEARCHER_ENTER);
#endif
  }

  /* Unlock the mutex so other searchers can enter */
  mutex_release(&list->searcher_mutex);

  return 0;
}

int llist_searcher_release(llist *list) {
  /* Locks mutex to update searcher_count */
  mutex_acquire(&list->searcher_mutex);

  list->searcher_count--;


  /* Only the last searcher unlocks the no_searcher semaphore */
  if (list->searcher_count == 0) {
#ifdef LOG
    llog_push(list->log, SEARCHER_LEAVE_LAST);
#endif
    sem_release(&list->no_searcher);
  } else {
#ifdef LOG
    llog_push(list->log, SEARCHER_LEAVE);
#endif
  }

  /* Unlock the mutex so other searchers can enter */
  mutex_release(&list->searcher_mutex);

  return 0;
}

/* An inserter can only enter if there are no deleters and no other inserters
 * holding the list */
int llist_inserter_acquire(llist *list) {
#ifdef LOG
  llog_push(list->log, INSERTER_WAIT);
#endif

  /* Since the deleter holds the no_inserter semaphore while it's active, we can
  use it as a way to find out if there is a deleter active */
  sem_acquire(&list->no_inserter);

#ifdef LOG
  llog_push(list->log, INSERTER_ENTER);
#endif

  return 0;
}

/* Since we're unlocking no_inserter before the inserter mutex, a waiting
 * deleter will get priority over a waiting inserter TODO (should we do this?)
 */
int llist_inserter_release(llist *list) {
#ifdef LOG
  llog_push(list->log, INSERTER_LEAVE);
#endif

  /* Signal that there are currently no inserters */
  sem_release(&list->no_inserter);

  return 0;
}

/* A deleter can only search if there are no searchers or inserters holding
 the list */
int llist_deleter_acquire(llist *list) {
#ifdef LOG
  llog_push(list->log, DELETER_WAIT);
#endif

  /* Wait until there are no searchers/inserters */
  sem_acquire(&list->no_searcher);
  sem_acquire(&list->no_inserter);
  /* NOTE by changing the order in which we wait, we change the averate wait
   * time for searchers and inserters. More precisely, whichever semaphore we
   * hold first will drive the corresponding wait time up. It doesn't seem to
   * affect the overall speed. */

#ifdef LOG
  llog_push(list->log, DELETER_ENTER);
#endif

  return 0;
}

/* Unlocks the no_searcher and no_inserter semaphores */
int llist_deleter_release(llist *list) {
#ifdef LOG
  llog_push(list->log, DELETER_LEAVE);
#endif

  /* Drop no_inserter and no_searchers semaphores */
  sem_release(&list->no_inserter);
  sem_release(&list->no_searcher);
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

  void *result = llist_find(ctx.list, ctx.value);

  llist_searcher_release(ctx.list);
  return result;
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
