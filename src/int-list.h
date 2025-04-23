#ifndef _INT_LIST_INCLUDE_H
#define _INT_LIST_INCLUDE_H
#define MAX_INT_LIST 100

#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>

typedef struct {
    size_t items[MAX_INT_LIST];
    size_t size;
} int_list;

void int_list_init(int_list *list);
int int_list_append(int_list *list, size_t value);
int int_list_remove(int_list *list, size_t value);

#endif