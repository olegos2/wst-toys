/*
 * Shared list internals. Backends expect LIST_ELEM, LIST_T, and LIST_F to be
 * defined. Set LIST_FMT to the element printf format, or define LIST_REF to
 * pass elements by pointer. Define LIST_IMPL in one translation unit per
 * stamp to emit definitions. A zero-initialized list is valid and empty.
 */

#ifndef TOYS_LIST_COMMON_INTERNAL_H
#define TOYS_LIST_COMMON_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>


#if !defined(LIST_USE_VERIFY)
#  ifdef WST_DEBUG
#    define LIST_USE_VERIFY 1
#  else
#    define LIST_USE_VERIFY 0
#  endif
#endif

/** Token-pasting and stringifying helpers. */
#define WST_LIST_CAT2(a, b) a ## b
#define WST_LIST_CAT(a, b) WST_LIST_CAT2(a, b)
#define WST_LIST_STR2(x) #x
#define WST_LIST_STR(x) WST_LIST_STR2(x)

/**
 * Error that may occur while working with a list.
 * Should be always handled after every operation.
 */
typedef enum {
    WST_LIST_NO_ERR = 0,
    /** Allocation failure, all operations except init keep contents. */
    WST_LIST_ERR_NOMEM,
    /** Pop from an empty list. */
    WST_LIST_ERR_EMPTY,
    /** Null list passed. */
    WST_LIST_ERR_NULL,
    /** Link, index, checksum, or length/capacity invariant is damaged. */
    WST_LIST_ERR_CORRUPT,
    /** Requested capacity does not fit the link width or size_t. */
    WST_LIST_ERR_OVERFLOW,
    /** Index is past the end of the list. */
    WST_LIST_ERR_BOUNDS,
} WstListErr;

/** Short human-readable description of a list error. */
const char *wst_list_err_str(WstListErr err);

/** A single untyped pointer-linked node header. */
struct WstListNodeHdr {
    struct WstListNodeHdr *next;
    struct WstListNodeHdr *prev;
};

typedef struct WstListNodeHdr WstListNodeHdr;

/** Pointer list header tracking head, tail, and linked free nodes. */
typedef struct WstListHdr {
    /** First live node, `NULL` when the list is empty. */
    WstListNodeHdr *head;
    /** Last live node, `NULL` when the list is empty. */
    WstListNodeHdr *tail;
    /** First free node. Free nodes are linked through `next`. */
    WstListNodeHdr *free;
} WstListHdr;

/** Untyped header at the start of every index-linked node. */
typedef struct {
    uint32_t next;
    uint32_t prev;
} WstListiNodeHdr;

/** Index list header. */
typedef struct {
    /** Node array, where slot 0 is a pseudo-node. */
    void *nodes;
    /** Allocated real node slots, excluding the sentinel. */
    size_t cap;
    /** Number of real slots ever allocated, including free ones. */
    uint32_t len;
    /** First free real node, or 0 when the free list is empty. */
    uint32_t free;
} WstListiHdr;

/** State of a slot in a list dump. */
typedef enum {
    WST_LIST_SLOT_ORPHAN = 0,
    WST_LIST_SLOT_LIVE,
    WST_LIST_SLOT_FREE,
} WstListSlotState;

/** Counts for each slot state. */
typedef struct {
    size_t live;
    size_t pooled;
    size_t orphan;
} WstListSlotCounts;

/** Short state name for dump tables. */
const char *wst_list_slot_state_name(WstListSlotState state);

/** Count live, pooled and orphan slots. */
WstListSlotCounts wst_list_slot_counts(const WstListSlotState *state,
                                       size_t count);

/** Formats one problem line and increments its count. */
typedef struct {
    FILE *stream;
    size_t count;
} WstListDiag;

/** Add one formatted problem line to `diag`. */
void wst_list_diag_add(WstListDiag *diag, const char *fmt, ...);

#endif /* TOYS_LIST_COMMON_INTERNAL_H */

/*
 * Stamped names shared by both backends. Kept outside the include guard
 * because LIST_T and LIST_F change between stamps.
 */
#ifdef LIST_T
#if !defined(LIST_ELEM)
#  error LIST_ELEM must be defined with the name of list element type
#endif

#if !defined(LIST_F)
#  error LIST_F must generate names for list impl functions
#endif

#if !defined(LIST_REF) && !defined(LIST_FMT)
#  error LIST_FMT must be a list element format string, or define LIST_REF
#endif

/* Public API names. */
#define list_free LIST_F(free)
#define list_clear LIST_F(clear)
#define list_init LIST_F(init)
#define list_reserve LIST_F(reserve)
#define list_verify LIST_F(verify)
#define list_dump LIST_F(dump)

#if LIST_USE_VERIFY
#  define WST_LIST_CHECK(list) list_verify(list)
#else
#  define WST_LIST_CHECK(list) WST_LIST_NO_ERR
#endif

#define list_get_node LIST_F(get_node)
#define list_get LIST_F(get)
#define list_remove_node LIST_F(remove_node)
#define list_remove LIST_F(remove)
#define list_pop LIST_F(pop)

#define list_find_node LIST_F(find_node)
#define list_find LIST_F(find)
#define list_set LIST_F(set)
#define list_append LIST_F(append)
#define list_insert_by_node LIST_F(insert_by_node)
#define list_insert LIST_F(insert)

/* Primitives provided by each backend to the shared typed API. */
#define list_node_at_raw LIST_F(node_at_raw)
#define list_find_node_raw LIST_F(find_node_raw)
#define list_find_index_raw LIST_F(find_index_raw)
#define list_append_raw LIST_F(append_raw)
#define list_insert_after_node_raw LIST_F(insert_after_node_raw)

/** Per-stamp typed node name. */
#define LIST_NODE_T WST_LIST_CAT(LIST_T, Node)
#endif /* LIST_T */

/*
 * Declarations used only by list implementations. The separate guard allows
 * LIST_IMPL to become defined between two includes in one translation unit.
 */
#ifdef LIST_IMPL
#ifndef TOYS_LIST_COMMON_IMPL_H
#define TOYS_LIST_COMMON_IMPL_H

/* Pointer backend helpers. */

/** Get node at `idx`, or `NULL` when out of bounds. */
WstListNodeHdr *wst_list_hdr_at(const WstListHdr *list, size_t idx);

/** Link `node` after `prev`. `prev == NULL` inserts at the front. */
void wst_list_hdr_insert_after(WstListHdr *list, WstListNodeHdr *prev,
                               WstListNodeHdr *node);

/** Unlink `node` and put it at the front of the free list. */
void wst_list_hdr_remove(WstListHdr *list, WstListNodeHdr *node);

/** Take a node from the free list, or return `NULL` when it is empty. */
WstListNodeHdr *wst_list_hdr_take_free(WstListHdr *list);

/** Reset endpoints to an empty list. */
void wst_list_hdr_reset(WstListHdr *list);

/** Put `count` nodes into the free list, keeping chain valid. */
void wst_list_hdr_add_free(WstListHdr *list, void *nodes, size_t count,
                           size_t stride);

/** Find first node whose data at `data_off` matches `value` bytewise. */
WstListNodeHdr *wst_list_hdr_find(const WstListHdr *list, size_t data_off,
                                  const void *value, size_t value_size);

/** Find index of the first bytewise match, or `-1` when not found. */
ssize_t wst_list_hdr_find_index(const WstListHdr *list, size_t data_off,
                                const void *value, size_t value_size);

/* Index backend helpers. */

/** Get node header at a logical index, or `NULL` when out of bounds. */
WstListiNodeHdr *wst_listi_hdr_at(const WstListiHdr *hdr, size_t stride,
                                  size_t idx);

/** Get the 0-based link for a node header. */
uint32_t wst_listi_hdr_index_of(const WstListiHdr *hdr, size_t stride,
                                const WstListiNodeHdr *node);

/** Get the last live link, or 0 when the list is empty. */
uint32_t wst_listi_hdr_tail(const WstListiHdr *hdr, size_t stride);

/** Allocate a free slot or grow the array, returning the new link in `out`. */
WstListErr wst_listi_hdr_alloc(WstListiHdr *hdr, size_t stride,
                               size_t init_cap, uint32_t *out);

/** Grow the node array to at least `count` real slots. */
WstListErr wst_listi_hdr_reserve(WstListiHdr *hdr, size_t stride,
                                 size_t count);

/** Link an already allocated node after `prev`, where slot 0 prepends. */
void wst_listi_hdr_insert_after(WstListiHdr *hdr, size_t stride, uint32_t prev,
                                uint32_t link);

/** Unlink a live node and put it at the front of the free chain. */
void wst_listi_hdr_remove(WstListiHdr *hdr, size_t stride, uint32_t link);

/** Reset the sentinel and free chain to an empty list. */
void wst_listi_hdr_reset(WstListiHdr *hdr, size_t stride);

/** Put a contiguous range of links into the free chain. */
void wst_listi_hdr_add_free(WstListiHdr *hdr, size_t stride, uint32_t first,
                            uint32_t count);

/** Free the node array and zero the header. */
void wst_listi_hdr_free(WstListiHdr *hdr);

/** Find first node whose data at `data_off` matches `value` bytewise. */
WstListiNodeHdr *wst_listi_hdr_find(const WstListiHdr *hdr, size_t stride,
                                    size_t data_off, const void *value,
                                    size_t value_size);

/** Find index of the first bytewise match, or `-1` when not found. */
ssize_t wst_listi_hdr_find_index(const WstListiHdr *hdr, size_t stride,
                                 size_t data_off, const void *value,
                                 size_t value_size);

/** Check the sentinel, index ranges and free-chain prefix invariants. */
WstListErr wst_listi_hdr_verify(const WstListiHdr *hdr, size_t stride);

#endif /* TOYS_LIST_COMMON_IMPL_H */
#endif /* LIST_IMPL */

