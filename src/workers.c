#include "sync.h"
#include "workers.h"
#include "int-list.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void state_print(llist *list) {
  state *s = &list->st;
  printf("__________________________________________________\n");
  llist_print(list);
  printf("STATUS:\n");
  for (size_t i = 0; i < s->searchers.size; i++) {
    printf("    Searching for %ld\n", s->searchers.items[i]);
  }
  if (s->inserters != 0) {
    printf("    Inserting %ld\n", s->inserters);
  }
  if (s->deleters != 0) {
    printf("    Deleting %ld\n", s->deleters);
  }
  printf("WAITING QUEUE:\n");
  printf("    Searchers waiting: %d\n    Inserters waiting: %d\n    Deleters waiting: %d\n",
        s->searchers_waiting, s->inserters_waiting, s->deleters_waiting);
}

int llist_searcher_acquire(llist_ctx *list_ctx) {
  /*
  A searcher can only search if there is no deleter currently holding the list.
  Initially, we simply add one to the counter searcher_count.
  Then, note that, in practice, we just need to acquire the no_searcher sempaphore,
  which informs other threads that at least one searcher is running and ensures it will
  not run concurrently with deleters.
  An important detail is to acquire this semaphore only when searcher_count == 1.
  */
  llist *list = list_ctx->list;
  #ifndef NDEBUG
  mutex_acquire(&list->st.lock);
  list->st.searchers_waiting++;
  state_print(list);
  mutex_release(&list->st.lock);
#endif // NDEBUG

  /* Lock the mutex to update searcher count */
  mutex_acquire(&list->searcher_mutex);

  list->searcher_count++;

  /* Only the first searcher locks the no_searcher semaphore */
  if (list->searcher_count == 1) {
    sem_acquire(&list->no_searcher);
  }

#ifndef NDEBUG
  /* We only check this *after* we've locked the no_searcher semaphore */
  mutex_acquire(&list->st.lock);
  list->st.searchers_waiting--;
  /* Now we can add to the end of the searchers list the new value */
  int_list_append(&list->st.searchers, list_ctx->value);
  /* Searchers cannot run concurrently with deleters */
  // assert(list->st.deleters == -1);
  /* At most one inserter can be active at a time */
  // assert(list->st.inserters <= 1);
  state_print(list);
  mutex_release(&list->st.lock);
#endif // NDEBUG

  /* Unlock the mutex so other searchers can enter */
  mutex_release(&list->searcher_mutex);

  return 0;
}

int llist_searcher_release(llist_ctx *list_ctx) {
  /*
  Decreases the searcher_count by one unit and, in case
  there are no searchers running anymore (i.e. searchers_count == 0),
  release the no_searcher semaphore to allow deleters to run.
  */

  llist *list = list_ctx->list;

  /* Locks mutex to update searcher_count */
  mutex_acquire(&list->searcher_mutex);

  list->searcher_count--;

#ifndef NDEBUG
  mutex_acquire(&list->st.lock);
  /* Remove the value from the searchers list */
  int_list_remove(&list->st.searchers, list_ctx->value);
  mutex_release(&list->st.lock);
#endif // NDEBUG

  /* Only the last searcher unlocks the no_searcher semaphore */
  if (list->searcher_count == 0) {
    sem_release(&list->no_searcher);
  }

  /* Unlock the mutex so other searchers can enter */
  mutex_release(&list->searcher_mutex);

  return 0;
}

int llist_inserter_acquire(llist_ctx *list_ctx) {
  /*
  An inserter can only run if there are no deleters and no other inserters
  holding the list.
  Hence, we simply wait until we can acquire the no_inserter sempahore.
  */
  llist *list = list_ctx->list;
#ifndef NDEBUG
  mutex_acquire(&list->st.lock);
  list->st.inserters_waiting++;
  state_print(list);
  mutex_release(&list->st.lock);
#endif // NDEBUG

  /* Since the deleter holds the no_inserter semaphore while it's active, we can
  use it as a way to find out if there is a deleter active */
  sem_acquire(&list->no_inserter);

#ifndef NDEBUG
  mutex_acquire(&list->st.lock);
  list->st.inserters = list_ctx->value;
  list->st.inserters_waiting--;
  state_print(list);
  /* At most one inserter can be active at a time */
  // assert(list->st.inserters <= 1);
  /* Inserters and deleters cannot run concurrently */
  // assert(list->st.deleters == 0);

  mutex_release(&list->st.lock);
#endif // NDEBUG

  return 0;
}

int llist_inserter_release(llist *list) {
  /*
  Simply release the no_inserter semaphore, informing the deleters threads
  that they can run.
  */
#ifndef NDEBUG
  mutex_acquire(&list->st.lock);

  /* Inserters may only run one at a time */
  // assert(list->st.inserters == 1);
  /* Inserters may not run concurrently with other deleters */
  // assert(list->st.deleters == -1);

  list->st.inserters = 0;
  mutex_release(&list->st.lock);
#endif // NDEBUG

  /* Signal that there are currently no inserters */
  sem_release(&list->no_inserter);

  return 0;
}

int llist_deleter_acquire(llist_ctx *list_ctx) {
  /*
  A deleter can only search if there are no searchers or inserters holding
  the list.
  Hence, we just wait until there are no searches and inserters by trying to
  acquire the no_searcher and no_inserter semaphores.
  */
  llist *list = list_ctx->list;
  #ifndef NDEBUG
  mutex_acquire(&list->st.lock);
  list->st.deleters_waiting++;
  state_print(list);
  mutex_release(&list->st.lock);
#endif // NDEBUG

  /* Wait until there are no searchers/inserters */
  sem_acquire(&list->no_searcher);
  sem_acquire(&list->no_inserter);

#ifndef NDEBUG
  mutex_acquire(&list->st.lock);
  list->st.deleters_waiting--;
  list->st.deleters = list_ctx->value;

  /* Deleters cannot run concurrently with deleters, inserters or searchers */
  // assert(list->st.deleters == -1);
  // assert(list->st.inserters == 0);
  state_print(list);
  mutex_release(&list->st.lock);
#endif // NDEBUG

  return 0;
}

/* Unlocks the no_searcher and no_inserter semaphores */
int llist_deleter_release(llist *list) {
  /*
  Release both no_inserter and no_searcher semaphores, indicating that
  the deleters thread have finished.
  */
#ifndef NDEBUG
  mutex_acquire(&list->st.lock);
  list->st.deleters = 0;

  /* Deleters cannot run concurrently with deleters, inserters or searchers */
  // assert(list->st.deleters == -1);
  // assert(list->st.inserters == -1);

  mutex_release(&list->st.lock);
#endif // NDEBUG

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
  /*
  Firstly, we acquire the necessary semaphores to properly run
  the searcher thread and tries to find the given value.
  */
  llist_ctx ctx = *(llist_ctx *)args;

  llist_searcher_acquire(&ctx);

  // Here, I am sure the searcher thread is running
  // Print the state showing that the searcher is currently searching for ctx->value

  void *result = llist_find(ctx.list, ctx.value);
  mutex_acquire(&ctx.list->st.lock);
  state_print(ctx.list);
  printf("RESULT:\n");
  if (result == NULL) {
    printf("    The value %ld is not on the list.\n", ctx.value);
  }
  else {
    printf("    The value %ld was found on the list!\n", ctx.value);
  }
  mutex_release(&ctx.list->st.lock);

  llist_searcher_release(&ctx);

  return result;
}

/* TODO: What can we return?
- Int: 0 for success, -1 on failure
- Enum: like int but clearer intent
- Pointer to value: Good but has the potential memory corruption problems
*/
void *inserter_thread(void *args) {
  llist_ctx ctx = *(llist_ctx *)args;

  llist_inserter_acquire(&ctx);

  llist_push_back(ctx.list, ctx.value);

  mutex_acquire(&ctx.list->st.lock);
  state_print(ctx.list);
  printf("RESULT:\n");
  printf("    The value %ld was inserted!\n", ctx.value);
  mutex_release(&ctx.list->st.lock);

  llist_inserter_release(ctx.list);

  return NULL;
}

/* TODO: What can we return?
 */
void *deleter_thread(void *args) {
  llist_ctx ctx = *(llist_ctx *)args;

  llist_deleter_acquire(&ctx);

  // TODO what happens when we can't delete?
  int result = llist_delete(ctx.list, ctx.value);

  mutex_acquire(&ctx.list->st.lock);
  state_print(ctx.list);
  printf("RESULT:\n");
  if (result == 0) {
    printf("    The value %ld was not deleted because it is not in the list!\n", ctx.value);
  }
  else {
    printf("    The value %ld was deleted!\n", ctx.value);
  }
  mutex_release(&ctx.list->st.lock);

  llist_deleter_release(ctx.list);

  return NULL;
}
