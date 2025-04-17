#define MAX_INT_LIST 100

#include <stdint.h>
#include <stdatomic.h>

typedef struct {
    int items[MAX_INT_LIST];
    size_t size;
} int_list;

void int_list_init(int_list *list);
int int_list_append(int_list *list, int value);
int int_list_get(int_list *list, size_t index);