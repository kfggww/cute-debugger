#ifndef LIST_H
#define LIST_H

typedef struct List {
    struct List *next;
    struct List *prev;
    void *data;
} List;

List *list_new();
void list_destroy(List *li);

#define list_init(li)                                                          \
    do {                                                                       \
        (li)->next = (li)->prev = li;                                          \
        (li)->data = NULL;                                                     \
    } while (0)

#define list_append(li, n)                                                     \
    do {                                                                       \
        n->next = li;                                                          \
        n->prev = (li)->prev;                                                  \
        (li)->prev = n;                                                        \
        n->prev->next = n;                                                     \
    } while (0)

#define list_prepend(li, n)                                                    \
    do {                                                                       \
        n->next = (li)->next;                                                  \
        n->prev = li;                                                          \
        (li)->next = n;                                                        \
        n->next->prev = n;                                                     \
    } while (0)

#define list_foreach(li, n) for (List *n = (li)->next; n != li; n = n->next)

#endif