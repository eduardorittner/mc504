#include "linked-list.h"
#include "sync.h"
#include "workers.h"
#include <stdio.h>
#include <stdlib.h>

#define INITIAL_SIZE 10000
#define SEARCHERS 1000
#define INSERTERS 100
#define DELETERS 100
int main(void) {
  pthread_t searchers[SEARCHERS];
  llist_ctx searchers_ctx[SEARCHERS];
  pthread_t inserters[INSERTERS];
  llist_ctx inserters_ctx[INSERTERS];
  pthread_t deleters[DELETERS];
  llist_ctx deleters_ctx[DELETERS];

  llist *list = llist_new();

  for (int i = 0; i < INITIAL_SIZE; i++) {
    llist_push_back(list, rand());
  }

  for (int i = 0; i < INSERTERS; i++) {
    inserters_ctx[i] = (llist_ctx){list, rand()};
    pthread_create(&inserters[i], NULL, inserter_thread,
                   (void *)&inserters_ctx[i]);
  }

  for (int i = 0; i < SEARCHERS; i++) {
    searchers_ctx[i] = (llist_ctx){list, rand()};
    pthread_create(&searchers[i], NULL, searcher_thread,
                   (void *)&searchers_ctx[i]);
  }

  for (int i = 0; i < DELETERS; i++) {
    deleters_ctx[i] = (llist_ctx){list, rand()};
    pthread_create(&deleters[i], NULL, deleter_thread,
                   (void *)&deleters_ctx[i]);
  }

  for (int i = 0; i < INSERTERS; i++) {
    pthread_join(inserters[i], NULL);
  }

  for (int i = 0; i < SEARCHERS; i++) {
    pthread_join(searchers[i], NULL);
  }

  for (int i = 0; i < DELETERS; i++) {
    pthread_join(deleters[i], NULL);
  }

  printf("All done\n");

  llist_print(list);
  llist_free(list);
  return 0;
}
