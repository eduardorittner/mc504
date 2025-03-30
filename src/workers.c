#include "workers.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void state_print(state *s) {
  printf("s: %d, s waiting: %d, i: %d, i waiting: %d, d: %d, d waiting: %d\n",
         s->searchers, s->searchers_waiting, s->inserters, s->inserters_waiting,
         s->deleters, s->deleters_waiting);
}

/* A searcher can only search if there is no deleter currently holding the list
 */
int llist_searcher_acquire(llist *list) {
  /* Debug state */
  pthread_mutex_lock(&list->st.lock);
  list->st.searchers_waiting++;
  state_print(&list->st);
  pthread_mutex_unlock(&list->st.lock);
  /* End of debug state */

  /* Lock the mutex to update searcher count */
  if (pthread_mutex_lock(&list->searcher_mutex) < 0)
    return -1;

  list->searcher_count++;

  /* Only the first searcher locks the no_searcher semaphore */
  if (list->searcher_count == 1) {
    if (sem_wait(&list->no_searcher) < 0)
      return -1;
  }

  /* Debug state */
  /* We only check this *after* we've locked the no_searcher semaphore */
  pthread_mutex_lock(&list->st.lock);
  list->st.searchers++;
  list->st.searchers_waiting--;

  /* Searchers cannot run concurrently with deleters */
  assert(list->st.deleters == 0);
  /* At most one inserter can be active at a time */
  assert(list->st.inserters <= 1);

  state_print(&list->st);
  pthread_mutex_unlock(&list->st.lock);
  /* End of debug state */

  /* Unlock the mutex so other searchers can enter */
  if (pthread_mutex_unlock(&list->searcher_mutex) < 0)
    return -1;

  return 0;
}

int llist_searcher_release(llist *list) {
  /* Locks mutex to update searcher_count */
  if (pthread_mutex_lock(&list->searcher_mutex) < 0)
    return -1;

  list->searcher_count--;

  /* Debug state */
  pthread_mutex_lock(&list->st.lock);
  list->st.searchers--;
  state_print(&list->st);
  pthread_mutex_unlock(&list->st.lock);
  /* End of debug state */

  /* Only the last searcher unlocks the no_searcher semaphore */
  if (list->searcher_count == 0) {
    if (sem_post(&list->no_searcher) < 0)
      return -1;
  }

  /* Unlock the mutex so other searchers can enter */
  if (pthread_mutex_unlock(&list->searcher_mutex) < 0)
    return -1;

  return 0;
}

/* An inserter can only enter if there are no deleters and no other inserters
 * holding the list */
int llist_inserter_acquire(llist *list) {
  /* Debug state */
  pthread_mutex_lock(&list->st.lock);
  list->st.inserters_waiting++;
  state_print(&list->st);
  pthread_mutex_unlock(&list->st.lock);
  /* End of debug state */

  /* Lock the inserter mutex, this mutex remains locked for the duration of the
   * inserter's lifetime */
  if (pthread_mutex_lock(&list->inserter) < 0)
    return -1;

  /* Since the deleter holds the no_inserter semaphore while it's active, we can
  use it as a way to find out if there is a deleter active */
  if (sem_wait(&list->no_inserter) < 0)
    return -1;

  /* Debug state */
  pthread_mutex_lock(&list->st.lock);
  list->st.inserters++;
  list->st.inserters_waiting--;

  /* At most one inserter can be active at a time */
  assert(list->st.inserters <= 1);
  /* Inserters and deleters cannot run concurrently */
  assert(list->st.deleters == 0);

  state_print(&list->st);
  pthread_mutex_unlock(&list->st.lock);
  /* End of debug state */

  return 0;
}

/* Since we're unlocking no_inserter before the inserter mutex, a waiting
 * deleter will get priority over a waiting inserter TODO (should we do this?)
 */
int llist_inserter_release(llist *list) {
  /* Debug state */
  pthread_mutex_lock(&list->st.lock);

  /* Inserters may only run one at a time */
  assert(list->st.inserters == 1);
  /* Inserters may not run concurrently with other deleters */
  assert(list->st.deleters == 0);

  list->st.inserters--;
  state_print(&list->st);
  pthread_mutex_unlock(&list->st.lock);
  /* End of debug state*/

  /* Signal that there are currently no inserters */
  if (sem_post(&list->no_inserter) < 0)
    return -1;

  /* Unlock the mutex so another inserter can lock it */
  if (pthread_mutex_unlock(&list->inserter) < 0)
    return -1;

  return 0;
}

/* A deleter can only search if there are no searchers or inserters holding
 the list */
int llist_deleter_acquire(llist *list) {
  /* Debug state */
  pthread_mutex_lock(&list->st.lock);
  list->st.deleters_waiting++;
  state_print(&list->st);
  pthread_mutex_unlock(&list->st.lock);
  /* End of debug state */

  /* Wait until there are no searchers/inserters */
  if (sem_wait(&list->no_searcher) < 0)
    return -1;
  if (sem_wait(&list->no_inserter) < 0)
    return -1;

  /* Debug state */
  pthread_mutex_lock(&list->st.lock);
  list->st.deleters_waiting--;
  list->st.deleters++;

  /* Deleters cannot run concurrently with deleters, inserters or searchers */
  assert(list->st.deleters == 1);
  // assert(list->st.inserters == 0);
  assert(list->st.searchers == 0);

  state_print(&list->st);
  pthread_mutex_unlock(&list->st.lock);
  /* End of debug state */

  return 0;
}

/* Unlocks the no_searcher and no_inserter semaphores */
int llist_deleter_release(llist *list) {
  /* Debug state */
  pthread_mutex_lock(&list->st.lock);
  list->st.deleters--;

  /* Deleters cannot run concurrently with deleters, inserters or searchers */
  assert(list->st.deleters == 0);
  assert(list->st.inserters == 0);
  assert(list->st.searchers == 0);

  state_print(&list->st);
  pthread_mutex_unlock(&list->st.lock);
  /* End of debug state */

  /* Drop no_inserter and no_searchers semaphores */
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
