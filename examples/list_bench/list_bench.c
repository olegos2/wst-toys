/*
 * Compare the two list backends on the same mixed workload.
 *
 * Build once without defines for the block backend and once with
 * `-DLIST_BENCH_INDEX` for the index backend. The defaults are intentionally
 * large enough to dominate process startup in a release build.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef LIST_BENCH_INDEX

#define LIST_ELEM int
#define LIST_T WstListIdxInt
#define LIST_F(name) wst_list_idx_int_ ## name
#define LIST_FMT "%d"
#define LIST_IMPL
#include "toys/impl/list_idx_internal.h"

#define BENCH_T WstListIdxInt
#define BENCH_F(name) wst_list_idx_int_ ## name
#define BENCH_NAME "index"

#else /* !LIST_BENCH_INDEX */

#include "toys/list.h"

#define BENCH_T WstListInt
#define BENCH_F(name) wst_list_int_ ## name
#define BENCH_NAME "block"

#endif /* LIST_BENCH_INDEX */

#ifndef LIST_BENCH_SIZE
#define LIST_BENCH_SIZE 50000
#endif

#ifndef LIST_BENCH_ROUNDS
#define LIST_BENCH_ROUNDS 4000
#endif

static void fail(const char *what)
{
    fprintf(stderr, "%s failed\n", what);
    exit(EXIT_FAILURE);
}

static void require(WstListErr err, const char *what)
{
    if (err != WST_LIST_NO_ERR) {
        fprintf(stderr, "%s: %s\n", what, wst_list_err_str(err));
        exit(EXIT_FAILURE);
    }
}

int main(void)
{
    BENCH_T list = { 0 };

    require(BENCH_F(init)(&list), "init");
    for (size_t i = 0; i < LIST_BENCH_SIZE; i++)
        require(BENCH_F(append)(&list, (int)i), "append");

    for (size_t round = 0; round < LIST_BENCH_ROUNDS; round++) {
        size_t index = (round * 7919) % LIST_BENCH_SIZE;
        int value = 0;

        if (!BENCH_F(get)(&list, index, &value))
            fail("get");

        int marker = (int)LIST_BENCH_SIZE + (int)round;
        if (!BENCH_F(set)(&list, index, marker))
            fail("set");
        if (BENCH_F(find)(&list, marker) < 0)
            fail("find");

        require(BENCH_F(insert)(&list, index, (int)round), "insert");
        if (!BENCH_F(remove)(&list, index))
            fail("remove");

        int popped = 0;
        require(BENCH_F(pop)(&list, &popped), "pop");
        require(BENCH_F(append)(&list, popped), "append");
    }

    require(BENCH_F(free)(&list), "free");
    printf("%s size=%d rounds=%d\n", BENCH_NAME, (int)LIST_BENCH_SIZE,
           (int)LIST_BENCH_ROUNDS);
    return EXIT_SUCCESS;
}
