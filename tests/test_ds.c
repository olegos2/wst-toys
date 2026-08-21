#include "toys/ds.h"

#include <stdio.h>

static int failures = 0;

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            failures++; \
        } \
    } while (0)


static void test_vec(void)
{
    int *v = NULL;

    /* empty vector reads as zero length */
    CHECK(vec_len(v) == 0, "null len");
    CHECK(vec_cap(v) == 0, "null cap");

    for (int i = 0; i < 100; i++)
        vec_push(v, i * 2);
    CHECK(vec_len(v) == 100, "len after pushes");
    CHECK(vec_cap(v) >= 100, "cap grew");

    bool ok = true;
    for (int i = 0; i < 100; i++)
        ok = ok && v[i] == i * 2;
    CHECK(ok, "pushed values");

    /* write by index */
    v[7] = -1;
    CHECK(v[7] == -1, "write by index");

    vec_free(v);
    CHECK(v == NULL, "free nulls the pointer");
    CHECK(vec_len(v) == 0, "freed len");
}

int main(void)
{
    test_vec();
    if (failures == 0) {
        printf("all tests passed\n");
        return 0;
    }
    printf("%d test(s) failed\n", failures);
    return 1;
}
