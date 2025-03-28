#include "linked-list.h"
#include "sync.h"
#include <stdio.h>

int main(void) {
  llist *list = llist_new();
  llist_print(list);
  llist_push_back(list, 26);
  llist_print(list);
  llist_push_back(list, 231);
  llist_delete(list, 31);
  llist_print(list);

  printf("node: %p, found: %p\n", list->head->next, llist_find(list, 231));
  return 0;
}
