#include "int-list.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

void int_list_init(int_list *list) {
    list->size = 0;
}

int int_list_append(int_list *list, size_t value) {
    if (list->size >= MAX_INT_LIST) return 0;
    list->items[list->size++] = value;
    return 1;
}

int int_list_remove(int_list *list, size_t value) {
    size_t i;
    for (i = 0; i < list->size; i++) {
        if (list->items[i] == value) {
            // Shift remaining elements to the left
            for (size_t j = i; j < list->size - 1; j++) {
                list->items[j] = list->items[j + 1];
            }
            list->size--;
            // Successfully removed
            return 1;
        }
    }
    // Value not found
    return 0;
}
