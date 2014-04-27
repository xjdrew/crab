#include <stdlib.h>
#include <stdint.h>

typedef struct _table_node {
    uint32_t key;
    int next;

    char flag; // 0: empty, 'n': non-terminator, 'o': terminator
    void* value;
} table_node;

typedef struct _table {
    int capacity;

    table_node* node;
    table_node* lastfree;
} table;

inline static void
initnode(table_node *node) {
    node->next = -1;

    node->flag = 0;
    node->value = NULL;
}

inline static int
tisnil(table_node* node) {
    return node->flag == 0;
}

inline static table_node*
tnode(table *t, int index) {
    return t->node + index;
}

inline static int
tindex(table *t, table_node *node) {
    return node - t->node;
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

static table_node*
table_newkey(table *t, uint32_t key);

static void
table_expand(table *t) {
    int capacity = t->capacity;
    table_node *node = t->node;

    t->capacity = t->capacity * 2;
    t->node = calloc(t->capacity, sizeof(table_node));
    int i;
    for(i=0;i<t->capacity;i++) {
        initnode(t->node + i);
    }
    for(i=0;i<capacity;i++) {
        table_node *old = capacity + i;
        if(tisnil(old)) {
            continue;
        }
        table_node *new = table_newkey(t, old->key);
        new->flag = old->flag;
        new->value = old->value;
    }
}

/*
** inserts a new key into a hash table; first, check whether key's main
** position is free. If not, check whether colliding node is in its main
** position or not: if it is not, move colliding node to an empty place and
** put new key in its main position; otherwise (colliding node is in its main
** position), new key goes to an empty position.
*/
static table_node*
table_newkey(table *t, uint32_t key) {
    table_node *mp = mainposition(t, key);
    if(!tisnil(mp)) {
        table_node *n = getfreenode(t);
        if(n == NULL) {
            table_expand(t);
            return table_newkey(t, key);
        }
        table_node *othern = mainposition(t, mp->key);
        if (othern != mp) {
            int mindex = tindex(t, mp);
            while(othern->next != mindex) {
                othern = tnode(t, othern->next);
            }
            othern->next = tindex(t, n);
            *n = *mp;
            initnode(mp);
        } else {
            n->next = mp->next;
            mp->next = tindex(t, n);
            mp = n;
        }
    }
    mp->key = key;
    mp->flag = 'n';
    return mp;
}

static table_node*
table_get(table *t, uint32_t key) {
    table_node *n = mainposition(t, key);
    while(!tisnil(n)) {
        if(n->key == key) {
            return n;
        }
        if(n->next < 0) {
            break;
        }
        n = tnode(t, n->next);
    }
    return NULL;
}

static table_node*
table_insert(table *t, uint32_t key) {
    table_node *node = table_get(t, key);
    if(node) {
        return node;
    }
    return table_newkey(t, key);
}

static table*
table_new(int capacity) {
    table *t = malloc(sizeof(table));
    t->capacity = 1;

    t->node = malloc(sizeof(table_node));
    initnode(t->node);
    t->lastfree = &t->node[0];
    return t;
}

static void
table_destory(table *t) {
}

// construct dictinory tree
static void
_dict_close(table *t) {
}

static int
_dict_insert(lua_State *L, table* dict) {
    if(!lua_istable(L, -1)) {
        return 0;
    }

    size_t len = lua_rawlen(L, 1);
    size_t i;
    uint32_t rune;
    table_node *node = NULL;
    for(i=1; i<=len; i++) {
        lua_rawgeti(L, -1, i);
        int issnum;
        rune = lua_tounsignedx(L, -1, &isnum);
        lua_pop(L, 1);

        if(!isnum) {
            return 0;
        }

        table *tmp;
        if(node == NULL) {
            tmp = dict;
        } else {
            if(node->value == NULL) {
                node->value = table_new();
            } 
            tmp = node->value;
        }
        node = table_insert(tmp, rune);
    }
    if(node) {
        node->flag = 'o';
    }
    return 1;
}

static int
dict_open(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    table *dict = table_new();
    size_t len = lua_rawlen(L,1);
    size_t i;
    for(i=1;i<=len;i++) {
        lua_rawgeti(L, 1, i);
        if(!_dict_insert(L, dict)) {
            _dict_close(dict);
            return luaL_error(L, "illegal parameters in table index %d", i);
        }
        lua_pop(L, 1);
    }

    // don't close old g_dict, avoid crash
    g_dict = dict;
    return 0;
}

static int
dict_filter(lua_State *L) {
    assert(g_dict);
    luaL_checktype(L, 1, LUA_TTABLE);

    size_t len = lua_rawlen(L,1);
    size_t i,j;
    int flag = 0;
    for(i=1;i<=len;) {
        table_node *node = NULL;
        int step = 0;
        for(j=i;j<=len;j++) {
            lua_rawgeti(L, 1, j);
            uint32_t rune = lua_tounsigned(L, 1, j);
            lua_pop(L, 1);

            if(node == NULL) {
                node = table_get(dict, rune);
            } else {
                node = table_get(node->value, rune);
            }

            if(node && node->flag == 'o') step = j - i + 1;
            if(!(node && node->value)) break;
        }
        if(step > 0) {
            for(j=0;j<step;j++) {
                lua_pushinteger(L, '*');
                lua_rawseti(L, 1, i+j);
            }
            flag = 1;
            i = i + step;
        } else {
            i++;
        }
    }
    lua_pushboolean(L, flag);
    return 1;
}

// interface
int
luaopen_crab_c(lua_State *L) {
    luaL_checkversion(L);

    luaL_Reg l[] = {
        {"open", dict_open},
        {"filter", dict_filter},
        {NULL, NULL}
    };

    luaL_newlib(L, l);
    return 1;
}