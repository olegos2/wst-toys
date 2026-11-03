#include <assert.h>

#ifdef LIST_T

/*
 * Value and reference stamps differ only in how an element is passed. These
 * helpers let the declarations and common wrappers be written once.
 */
#ifdef LIST_REF
#  define WST_LIST_ARG_TYPE const LIST_ELEM *
#  define WST_LIST_ARG_DEREF(val) (val)
#  define WST_LIST_ARG_ASSERT(val) assert((val) != NULL)
#else /* !LIST_REF */
#  define WST_LIST_ARG_TYPE LIST_ELEM
#  define WST_LIST_ARG_DEREF(val) (&(val))
#  define WST_LIST_ARG_ASSERT(val) ((void)0)
#endif /* LIST_REF */

/* Shared declarations. */

/** Free list memory and reset all fields. Refuses on verification failure. */
WstListErr list_free(LIST_T *list);

/**
 * Clear all elements, keep allocation/capacity. Refuses on verification
 * failure.
 */
WstListErr list_clear(LIST_T *list);

/** Initialize list as empty, dropping any previous allocation. */
WstListErr list_init(LIST_T *list);

/** Ensure storage for at least `count` elements. */
WstListErr list_reserve(LIST_T *list, size_t count);

/** Check the list invariants. */
WstListErr list_verify(const LIST_T *list);

/** Dump all debug information about list and its contents. */
void list_dump(const LIST_T *list);

/**
 * Get a node by logical index. Returns `NULL` when not found.
 */
LIST_NODE_T *list_get_node(const LIST_T *list, size_t idx);

/** Get value by index. Returns `false` when index is out of bounds. */
bool list_get(const LIST_T *list, size_t idx, LIST_ELEM *out);

/** Remove list element by its node pointer, which must belong to `list`. */
bool list_remove_node(LIST_T *list, const LIST_NODE_T *node);

/** Remove list element by index. */
bool list_remove(LIST_T *list, size_t idx);

/** Pop list back (last element). */
WstListErr list_pop(LIST_T *list, LIST_ELEM *out);

/** Find list node by value (bytewise). Returns `NULL` when not found. */
LIST_NODE_T *list_find_node(const LIST_T *list, WST_LIST_ARG_TYPE val);

/** Find list element index by value (bytewise). Returns `-1` when not found. */
ssize_t list_find(const LIST_T *list, WST_LIST_ARG_TYPE val);

/**
 * Set list element value by index. Returns `false` when index is out of
 * bounds.
 */
bool list_set(LIST_T *list, size_t idx, WST_LIST_ARG_TYPE val);

/** Append value to back (tail) of the list. */
WstListErr list_append(LIST_T *list, WST_LIST_ARG_TYPE val);

/**
 * Insert a new element after `node`, which must belong to `list`.
 * `NULL` prepends to the list.
 */
WstListErr list_insert_by_node(LIST_T *list, const LIST_NODE_T *node,
                               WST_LIST_ARG_TYPE val);

/** Insert a new element at `idx`, shifting elements at and after it back. */
WstListErr list_insert(LIST_T *list, size_t idx, WST_LIST_ARG_TYPE val);

/* Shared implementations. */
#ifdef LIST_IMPL

LIST_NODE_T *list_get_node(const LIST_T *list, size_t idx)
{
    assert(list != NULL);

    if (WST_LIST_CHECK(list) != WST_LIST_NO_ERR)
        return NULL;

    return list_node_at_raw(list, idx);
}

bool list_get(const LIST_T *list, size_t idx, LIST_ELEM *out)
{
    assert(out != NULL);

    const LIST_NODE_T *node = list_get_node(list, idx);
    if (node == NULL)
        return false;
    *out = node->data;
    return true;
}

bool list_remove(LIST_T *list, size_t idx)
{
    const LIST_NODE_T *node = list_get_node(list, idx);
    if (node == NULL)
        return false;
    return list_remove_node(list, node);
}

LIST_NODE_T *list_find_node(const LIST_T *list, WST_LIST_ARG_TYPE val)
{
    assert(list != NULL);
    WST_LIST_ARG_ASSERT(val);

    if (WST_LIST_CHECK(list) != WST_LIST_NO_ERR)
        return NULL;

    return list_find_node_raw(list, WST_LIST_ARG_DEREF(val));
}

ssize_t list_find(const LIST_T *list, WST_LIST_ARG_TYPE val)
{
    assert(list != NULL);
    WST_LIST_ARG_ASSERT(val);

    if (WST_LIST_CHECK(list) != WST_LIST_NO_ERR)
        return -1;

    return list_find_index_raw(list, WST_LIST_ARG_DEREF(val));
}

bool list_set(LIST_T *list, size_t idx, WST_LIST_ARG_TYPE val)
{
    WST_LIST_ARG_ASSERT(val);

    LIST_NODE_T *node = list_get_node(list, idx);
    if (node == NULL)
        return false;
    node->data = *WST_LIST_ARG_DEREF(val);
    return true;
}

WstListErr list_append(LIST_T *list, WST_LIST_ARG_TYPE val)
{
    assert(list != NULL);
    WST_LIST_ARG_ASSERT(val);

    WstListErr err = WST_LIST_CHECK(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    return list_append_raw(list, WST_LIST_ARG_DEREF(val));
}

WstListErr list_insert_by_node(LIST_T *list, const LIST_NODE_T *node,
                               WST_LIST_ARG_TYPE val)
{
    assert(list != NULL);
    WST_LIST_ARG_ASSERT(val);

    WstListErr err = WST_LIST_CHECK(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    return list_insert_after_node_raw(list, node, WST_LIST_ARG_DEREF(val));
}

WstListErr list_insert(LIST_T *list, size_t idx, WST_LIST_ARG_TYPE val)
{
    assert(list != NULL);
    WST_LIST_ARG_ASSERT(val);

    WstListErr err = WST_LIST_CHECK(list);
    if (err != WST_LIST_NO_ERR)
        return err;

    const LIST_NODE_T *prev = NULL;
    if (idx > 0) {
        prev = list_node_at_raw(list, idx - 1);
        if (prev == NULL)
            return WST_LIST_ERR_BOUNDS;
    }

    return list_insert_after_node_raw(list, prev, WST_LIST_ARG_DEREF(val));
}

#endif /* LIST_IMPL */

#undef WST_LIST_ARG_TYPE
#undef WST_LIST_ARG_DEREF
#undef WST_LIST_ARG_ASSERT

#endif /* LIST_T */
