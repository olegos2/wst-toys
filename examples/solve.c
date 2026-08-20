#include "toys/solve.h"
#include "toys/debug.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

/** strtod on str[0..len) (need not be NUL-terminated), exactly one double. */
static bool parse_coeff(const char *str, size_t len, double *out)
{
    char *end = NULL;
    double val = strtod(str, &end);
    if (end == str || end != str + len)
        return false;
    *out = val;
    return true;
}

/** Print real equation solutions, mapping special `TOYS_SOLVE_*` constants. */
static void print_solution(int count, const double *roots)
{
    if (count == TOYS_SOLVE_INF) {
        printf("infinite solutions\n");
        return;
    }
    if (count == TOYS_SOLVE_ERR) {
        printf("solver error\n");
        return;
    }
    if (count == 0) {
        printf("no real roots\n");
        return;
    }
    printf("%d roots:\n", count);
    for (int i = 0; i < count; i++)
        printf("x%d = %g\n", i + 1, roots[i]);
}

/** Skip space, tab and newline at s. */
static char *skip_seps(char *s)
{
    while (*s == ' ' || *s == '\t' || *s == '\n')
        s++;
    return s;
}

/** First whitespace or NUL in s. */
static char *token_end(char *s)
{
    while (*s != ' ' && *s != '\t' && *s != '\n' && *s != '\0')
        s++;
    return s;
}
/**
 * Parse a line into a malloc'd double array: first count the
 * whitespace-delimited numbers to size the allocation, then parse each
 * token by length with parse_coeff. The line is never modified.
 */
static double *parse_line(char *line, int *count)
{
    int n = 0;
    for (char *p = skip_seps(line); *p != '\0';
         p = skip_seps(token_end(p)))
        n++;

    if (n == 0)
        return NULL;

    double *coeffs = malloc((size_t)n * sizeof(double));
    if (coeffs == NULL) {
        LOG_E("Failed to allocate coeffs: %s", strerror(errno));
        return NULL;
    }

    int i = 0;
    char *p = skip_seps(line);
    while (*p != '\0') {
        char *end = token_end(p);
        if (!parse_coeff(p, (size_t)(end - p), &coeffs[i++])) {
            LOG_E("invalid number %.*s", (int)(end - p), p);
            free(coeffs);
            return NULL;
        }
        p = skip_seps(end);
    }

    *count = i;
    return coeffs;
}

/**
 * Read coefficient lines from stdin until EOF and print results per line.
 */
static void run_interactive(void)
{
    if (isatty(fileno(stdin))) {
        printf("Polynomial equation solver for real roots.\n");
        printf("Type the coefficients from the constant term up to the highest power of x,\n");
        printf("separated by spaces, then press Enter.\n");
        printf("  example:  1 -6 11 -6   solves  x^3 - 6 x^2 + 11 x - 6 = 0\n");
        printf("An empty line is skipped; Ctrl-D exits.\n");
    }

    char *line = NULL;
    size_t len = 0;
    while (1) {
        if (isatty(fileno(stdin))) {
            printf("> ");
            fflush(stdout);
        }
        if (getline(&line, &len, stdin) == -1)
            break;

        int count;
        double *coeffs = parse_line(line, &count);
        if (!coeffs)
            continue;

        double roots[TOYS_POLY_MAX_DEGREE + 1];
        LOG_D("solving degree %d from stdin", count - 1);
        int n = toys_poly_solve(count - 1, coeffs, roots, TOYS_POLY_MAX_DEGREE + 1);
        print_solution(n, roots);
        free(coeffs);
    }
    free(line);
}


int main(int argc, char *argv[])
{
    if (argc == 1) {
        run_interactive();
        return 0;
    }

    double *coeffs = malloc((size_t)(argc - 1) * sizeof(double));
    if (coeffs == NULL) {
        LOG_E("Failed to allocate coeffs: %s", strerror(errno));
        return 1;
    }

    int count = 0;
    for (int i = 1; i < argc; i++) {
        if (!parse_coeff(argv[i], strlen(argv[i]), &coeffs[count])) {
            LOG_E("invalid coefficient %s", argv[i]);
            free(coeffs);
            return 1;
        }
        count++;
    }

    LOG_D("solving degree %d from argv", count - 1);
    double roots[TOYS_POLY_MAX_DEGREE + 1];
    int n = toys_poly_solve(count - 1, coeffs, roots, TOYS_POLY_MAX_DEGREE + 1);
    print_solution(n, roots);

    free(coeffs);
    return 0;
}
