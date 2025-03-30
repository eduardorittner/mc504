#include "../src/linked-list.h"
#include "greatest.h"

/* Assert that creating and freeing a linked list incurs no memory leaks, note
 * that this test may pass but still trigger the address sanitizer, which is
 * what we are looking for */
TEST create_empty_list(void) {
  llist *list = llist_new();
  llist_free(list);
  PASS();
}

TEST insert_simple(void) {
  llist *list = llist_new();
  llist *expected = llist_new();

  lnode *node5 = lnode_new(11);
  lnode *node4 = lnode_new(120);
  node4->next = node5;
  lnode *node3 = lnode_new(23);
  node3->next = node4;
  lnode *node2 = lnode_new(52);
  node2->next = node3;
  lnode *node1 = lnode_new(21);
  node1->next = node2;
  expected->head = node1;

  llist_push_back(list, 21);
  llist_push_back(list, 52);
  llist_push_back(list, 23);
  llist_push_back(list, 120);
  llist_push_back(list, 11);

  lnode *expected_node = expected->head;
  lnode *actual_node = list->head;
  for (int i = 0; i < 5; i++) {
    ASSERT_EQ_FMT(actual_node->value, expected_node->value, "%zu");
    actual_node = actual_node->next;
    expected_node = expected_node->next;
  }

  llist_free(expected);
  llist_free(list);
  PASS();
}

TEST delete_empty_list(void) {
  llist *list = llist_new();
  ASSERT_LT(llist_delete(list, 1), 0);
  llist_free(list);
  PASS();
}

/* Insert some values and delete all of them resulting in an empty list */
TEST insert_and_delete(void) {
  llist *list = llist_new();
  llist_push_back(list, 10);
  llist_push_back(list, 23);
  llist_push_back(list, 102);

  llist_delete(list, 23);
  llist_delete(list, 102);
  llist_delete(list, 10);
  ASSERT_EQ(list->head, NULL);

  llist_free(list);

  PASS();
}

TEST find_simple(void) {
  llist *list = llist_new();
  llist_push_back(list, 10);
  llist_push_back(list, 26);
  llist_push_back(list, 54);

  ASSERT_EQ(llist_find(list, 10), list->head);
  ASSERT_EQ(llist_find(list, 54), list->head->next->next);
  ASSERT_EQ(llist_find(list, 27), NULL);

  llist_free(list);
  PASS();
}

SUITE(main_suite) {
  RUN_TEST(create_empty_list);
  RUN_TEST(insert_simple);
  RUN_TEST(delete_empty_list);
  RUN_TEST(insert_and_delete);
  RUN_TEST(find_simple);
}

/* Add definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN(); /* command-line options, initialization. */

  RUN_SUITE(main_suite);

  GREATEST_MAIN_END(); /* display results */
}
