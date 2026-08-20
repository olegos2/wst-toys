#include "toys/debug.h"
#include "toys/expr.h"
#include "toys/solve.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/* strtod on str[0..len); must consume the whole span as one double. */
static bool parse_coeff(const char *str, size_t len, double *out)
{
    char *end = NULL;
    double val = strtod(str, &end);
    if (end == str || end != str + len)
        return false;
    *out = val;
    return true;
}

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

static char *skip_seps(char *s)
{
    while (*s == ' ' || *s == '\t' || *s == '\n')
        s++;
    return s;
}

static char *token_end(char *s)
{
    while (*s != ' ' && *s != '\t' && *s != '\n' && *s != '\0')
        s++;
    return s;
}

/* Two passes: count whitespace-delimited numbers to size the
 * allocation, then parse each one by length. The line is never modified. */
static int parse_line(char *line, ToysPoly *poly)
{
    int n = 0;
    for (char *p = skip_seps(line); *p != '\0'; p = skip_seps(token_end(p)))
        n++;
    if (n == 0)
        return 0;
    if (n - 1 > TOYS_POLY_MAX_DEGREE) {
        LOG_E("too many coefficients on one line (max degree %d)",
              TOYS_POLY_MAX_DEGREE);
        return 0;
    }

    int i = 0;
    for (char *p = skip_seps(line); *p != '\0';) {
        char *end = token_end(p);
        if (!parse_coeff(p, (size_t)(end - p), &poly->coeffs[i])) {
            LOG_E("invalid number %.*s", (int)(end - p), p);
            return 0;
        }
        poly->degree = i;
        i++;
        p = skip_seps(end);
    }
    return 1;
}

static void solve_and_print(const ToysPoly *poly, const char *source)
{
    LOG_D("solving degree %d %s", poly->degree, source);

    double roots[TOYS_POLY_MAX_DEGREE + 1];
    int count = toys_poly_solve(poly, roots, TOYS_POLY_MAX_DEGREE + 1);
    print_solution(count, roots);
}

/* Reads coefficient lines from stdin until EOF and prints results. */
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

        ToysPoly poly = { 0 };
        if (!parse_line(line, &poly))
            continue;
        solve_and_print(&poly, "from stdin");
    }
    free(line);
}


static void print_help(void)
{
    printf(
        "Usage: toys_solve [options] <command> [args...]\n"
        "\n"
        "Solve a polynomial equation for its real roots.\n"
        "\n"
        "Commands:\n"
        "  coeffs c0 c1 ... cn  solve c0 + c1 x + ... + cn x^n = 0 with the\n"
        "                       given numeric coefficients\n"
        "  expr expression      solve a mathematical expression set equal to\n"
        "                       zero, e.g. x^2 - 4 = 0 or x^3 - 6x^2 + 11x - 6\n"
        "  (none)               interactive mode, reads coefficient lines from\n"
        "                       stdin\n"
        "\n"
        "Options:\n"
        "  -h, --help  print this help and exit\n");
}

/* `coeffs` command: solve using numeric coefficients from argv[1..]. */
static int run_coeffs(int argc, char *argv[])
{
    if (argc - 1 > TOYS_POLY_MAX_DEGREE + 1) {
        LOG_E("too many coefficients (max degree %d)", TOYS_POLY_MAX_DEGREE);
        return 1;
    }

    ToysPoly poly = { 0 };
    int count = 0;
    for (int i = 1; i < argc; i++) {
        if (!parse_coeff(argv[i], strlen(argv[i]), &poly.coeffs[count])) {
            LOG_E("invalid coefficient %s", argv[i]);
            return 1;
        }
        poly.degree = count;
        count++;
    }

    solve_and_print(&poly, "from argv");
    return 0;
}

/* `expr` command: join remaining args into one expression string. */
static int run_expr(int argc, char *argv[])
{
    if (argc <= 1) {
        LOG_E("no expression given, run 'toys_solve --help' for usage");
        return 1;
    }

    size_t total = 1;
    for (int i = 1; i < argc; i++)
        total += strlen(argv[i]) + 1;

    char *expr = malloc(total);
    if (expr == NULL) {
        LOG_E("Failed to allocate expression: %s", strerror(errno));
        return 1;
    }

    char *dst = expr;
    for (int i = 1; i < argc; i++) {
        if (i > 1)
            *dst++ = ' ';
        size_t n = strlen(argv[i]);
        memcpy(dst, argv[i], n);
        dst += n;
    }
    *dst = '\0';

    ToysPoly poly;
    size_t err_pos;
    const char *err_msg;
    if (!toys_expr_to_poly(expr, strlen(expr), &poly, &err_pos, &err_msg)) {
        fprintf(stderr, "expr error at position %zu: %s\n", err_pos, err_msg);
        free(expr);
        return 1;
    }
    free(expr);

    solve_and_print(&poly, "from expr");
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        run_interactive();
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        }
    }

    const char *command = argv[1];
    if (strcmp(command, "coeffs") == 0)
        return run_coeffs(argc - 1, argv + 1);
    if (strcmp(command, "expr") == 0)
        return run_expr(argc - 1, argv + 1);

    LOG_E("unknown command '%s', run 'toys_solve --help' for usage", command);
    return 1;
}