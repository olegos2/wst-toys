#ifndef TESTS_COMMON_H
#define TESTS_COMMON_H

#include "toys/math.h"
#include "toys/poly.h"

#include <stdio.h>

#define ARR_LEN(arr) (sizeof(arr) / sizeof(*arr))

static int failures = 0;

#define FAIL(fmt, ...) \
    do { \
        fprintf(stderr, "FAIL (%s:%s:%d) " fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__); \
        failures++; \
    } while (0)

#define CHECK(cond, ...) \
    do { if (!(cond)) FAIL(__VA_ARGS__); } while (0)


static void print_poly(const WstPoly *poly, const char *name)
{
    fprintf(stderr, "%s degree %d:\n", name, poly->degree);
    for (int i = 0; i <= poly->degree; i++)
        fprintf(stderr, "[%d] = %lg,\n", i, poly->coeffs[i]);
}

static void print_sol(const WstSolution *sol, const char *name)
{
    fprintf(stderr, "%s %d roots:\n", name, sol->count);
    for (int i = 0; i < sol->count; i++)
        fprintf(stderr, "[%d] = %lg,\n", i, sol->roots[i]);
}

static bool cmp_solution(const WstSolution *s, const WstSolution *expected)
{
    if (s->count != expected->count)
        return false;

    for (int i = 0; i < s->count; i++) {
        if (!my_iszero(s->roots[i] - expected->roots[i]))
            return false;
    }

    return true;
}

static int tests_summary(void)
{
    if (failures == 0) {
        printf("all tests passed\n");
        return 0;
    }
    printf("%d test(s) failed\n", failures);
    return 1;
}

#endif /* TESTS_COMMON_H */
