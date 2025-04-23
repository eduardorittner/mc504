#include "linked-list.h"
#include "sync.h"
#include "int-list.h"
#include <stdio.h>
#include <stdlib.h>

llist *llist_new(void) {
  llist *list = calloc(1, sizeof(*list));
  mutex_new(&list->searcher_mutex);
  mutex_new(&list->st.lock);
  sem_new(&list->no_searcher, 1);
  sem_new(&list->no_inserter, 1);
  list->head = NULL;
  int_list_init(&list->st.searchers);
  list->st.searchers_waiting = 0;
  list->st.inserters = 0;
  list->st.inserters_waiting = 0;
  list->st.deleters = 0;
  list->st.deleters_waiting = 0;
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
  pthread_mutex_destroy(&list->st.lock);
  sem_destroy(&list->no_searcher);
  sem_destroy(&list->no_inserter);
  free(list);
}

void llist_print(llist *list) {
  lnode *current = list->head;

  while (current != NULL) {
      printf("%zu", current->value);
      if (current->next != NULL) {
          printf(" -> ");
      }
      current = current->next;
  }
  printf("\n");
}

void llist_push_back(llist *list, size_t value) {
  /*
  Append the value to the end of the linked list.
  */
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
  /*
  This function tries to delete the first element equals to value in the list.
  If such value is found, the function deletes the respective node from the list and
  returns 1.
  Otherwise, i.e. if there is no such element, the function returns 0.
  */
  lnode **cur = &list->head;

  while ((*cur) != NULL) {
    if ((*cur)->value == value) {
      lnode *deleted = *cur;
      *cur = (*cur)->next;
      lnode_free(deleted);
      return 1;
    }
    cur = &(*cur)->next;
  }

  return 0;
}

lnode *llist_find(llist *list, size_t value) {
  /*
  Searches for value in the given list.
  If value is in the list, return a pointer to the node containing the value;
  Otherwise, return NULL.
  */
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
