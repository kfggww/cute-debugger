#include "stdlib.h"
#include "list.h"

List *list_new() {
    List *li = malloc(sizeof(*li));
    list_init(li);
    return li;
}
void list_destroy(List *li) {
    List *node = li->next;
    List *node1 = NULL;

    while (node != li) {
        node1 = node->next;
        if (node) {
            if (node->data) {
                free(node->data);
            }
            free(node);
        }
        node = node1;
    }

    free(li);
}
