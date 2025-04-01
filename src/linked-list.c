#include "linked-list.h"
#include "log.h"
#include "sync.h"
#include <stdio.h>
#include <stdlib.h>

llist *llist_new(size_t log_len) {
  llist *list = calloc(1, sizeof(*list));
  list->log = llog_new(log_len);
  sem_new(&list->no_searcher, 1);
  sem_new(&list->no_inserter, 1);
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

  pthread_mutex_destroy(&list->searcher_mutex);
  sem_destroy(&list->no_searcher);
  sem_destroy(&list->no_inserter);
  llog_free(list->log);
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
