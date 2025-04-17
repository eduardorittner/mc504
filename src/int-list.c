#include <int-list.h>
#include <stdio.h>
#include <stdlib.h>

void int_list_init(int_list *list) {
    list->size = 0;
}

int int_list_append(int_list *list, int value) {
    if (list->size >= MAX_INT_LIST) return 0;
    list->items[list->size++] = value;
    return 1;
}

int int_list_get(int_list *list, size_t index) {
    if (index >= list->size) return -1;
    return list->items[index];
}