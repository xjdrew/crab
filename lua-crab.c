#include <stdlib.h>
#include <stdint.h>

typedef struct _table_node {
    uint32_t key;
    int next;

    char flag; // 0: empty, 'n': non-terminator, 't': terminator
    void* value;
} table_node;

typedef struct _table {
    int capacity;

    table_node* node;
    table_node* lastfree;
} table;

inline static int
tisnil(table_node* node) {
    return node->flag == 0;
}

static table_node*
mainposition(table *t, uint32_t key) {
    return &t->node[(key & (t->capacity -1))];
}

static table_node*
getfreenode(table *t) {
    while(t->lastfree >= t->node) {
        if(tisnil(t->lastfree)) {
            return t->lastfree;
        }
        t->lastfree--;
    }
    return NULL;
}

static void
table_expend(table *t) {
}

/*
** inserts a new key into a hash table; first, check whether key's main
** position is free. If not, check whether colliding node is in its main
** position or not: if it is not, move colliding node to an empty place and
** put new key in its main position; otherwise (colliding node is in its main
** position), new key goes to an empty position.
*/
static table_node*
table_insert(table *t, uint32_t key) {
    table_node *mp = mainposition(t, key);
    if(!tisnil(mp)) {
        table_node *othern;
        table_node *n = getfreenode(t);
        if(n == NULL) {
            table_expend(t);
            return table_insert(t, key);
        }

    }
    mp->key = key;
    mp->next = -1;
    mp->flag = 'n';
    mp->value = NULL;
    return mp;
}

static table_node*
table_get(table *t, uint32_t key) {
}

static table*
table_new(int capacity) {
    table *t = malloc(sizeof(table));
    t->capacity = 1;

    t->node = malloc(sizeof(table_node));
    t->node[0].flag = 0;
    t->lastfree = &t->node[0];
    return t;
}

