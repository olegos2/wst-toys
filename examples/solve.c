#include "toys/debug.h"
#include "toys/solve.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(_MSC_VER) || defined(_WIN32)
#  include <io.h>
#  define isatty _isatty
#  define fileno _fileno
#else
#  include <unistd.h>
#endif

/* strtod must consume the whole string as one number. */
static bool parse_coeff(const char *str, size_t len, double *out)
{
    assert(out != NULL);

    char *end = NULL;
    double val = strtod(str, &end);
    if (end == str || end != str + len)
        return false;
    *out = val;
    return true;
}

static void print_solution(const WstSolution *sol)
{
    assert(sol != NULL);

    if (sol->count == WST_SOLVE_INF) {
        printf("infinite solutions\n");
        return;
    }
    if (sol->count == WST_SOLVE_ERR) {
        printf("solver error\n");
        return;
    }
    if (sol->count == 0) {
        printf("no real roots\n");
        return;
    }

    printf("%d roots:\n", sol->count);
    for (int i = 0; i < sol->count; i++)
        printf("x%d = %lg\n", i + 1, sol->roots[i] + 0.0); // workaround to avoid printing -0
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

/* First pass counts the numbers to enforce the degree cap. */
static bool parse_line(char *line, WstPoly *poly)
{
    assert(line != NULL);

    int n = 0;
    for (char *p = skip_seps(line); *p != '\0'; p = skip_seps(token_end(p)))
        n++;
    if (n == 0)
        return false;
    if (n - 1 > WST_SOLVE_MAX_DEGREE) {
        fprintf(stderr, "Too many coefficients on one line (max degree %d)\n",
                WST_SOLVE_MAX_DEGREE);
        return false;
    }

    int i = 0;
    for (char *p = skip_seps(line); *p != '\0';) {
        char *end = token_end(p);
        if (!parse_coeff(p, (size_t)(end - p), &poly->coeffs[i])) {
            fprintf(stderr, "Invalid number %.*s\n", (int)(end - p), p);
            return false;
        }
        poly->degree = i;
        i++;
        p = skip_seps(end);
    }
    return true;
}

/* Reads coefficient lines from stdin until EOF and prints results. */
static void run_interactive(void)
{
    if (isatty(fileno(stdin))) {
        printf("Polynomial equation solver for real roots.\n");
        printf("Type the coefficients from the constant term up to the highest power of x,\n");
        printf("separated by spaces, then press Enter.\n");
        printf("  example:  2 -3 1   solves  x^2 - 3 x + 2 = 0\n");
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

        WstPoly poly = { 0 };
        if (!parse_line(line, &poly))
            continue;
        WstSolution sol = wst_solve_poly(&poly);
        print_solution(&sol);
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
        "                       zero, e.g. x^2 - 4 = 0 or 2 - x^2 = 0\n"
        "  (none)               interactive mode, reads coefficient lines from\n"
        "                       stdin\n"
        "\n"
        "Options:\n"
        "  -h, --help  print this help and exit\n");
}

/* coeffs command: solve using numeric coefficients from argv. */
static int run_coeffs(int argc, char *argv[])
{
    if (argc - 1 > WST_SOLVE_MAX_DEGREE + 1) {
        LOG_E("too many coefficients (max degree %d)", WST_SOLVE_MAX_DEGREE);
        return 1;
    }

    /* Parse coefficients separately, with malformed numbers checks. */
    WstPoly poly = { 0 };
    int count = 0;
    for (int i = 1; i < argc; i++) {
        if (!parse_coeff(argv[i], strlen(argv[i]), &poly.coeffs[count])) {
            LOG_E("invalid coefficient %s", argv[i]);
            return 1;
        }
        poly.degree = count;
        count++;
    }

    WstSolution sol = wst_solve_poly(&poly);
    print_solution(&sol);
    return 0;
}

/* expr command */
static int run_expr(int argc, char *argv[])
{
    if (argc <= 1) {
        fprintf(stderr, "No expression given, run 'toys_solve --help' for usage\n");
        return 1;
    }

    /* Count space needed for remaining args to join in one string. */
    size_t total = 1;
    for (int i = 1; i < argc; i++)
        total += strlen(argv[i]) + 1;

    char *expr = malloc(total);
    if (expr == NULL) {
        LOG_E("Failed to allocate expression: %s", strerror(errno));
        return 1;
    }

    /* Copy args into one string */
    char *dst = expr;
    for (int i = 1; i < argc; i++) {
        if (i > 1)
            *dst++ = ' ';
        size_t n = strlen(argv[i]);
        memcpy(dst, argv[i], n);
        dst += n;
    }
    *dst = '\0';

    WstPoly poly;
    size_t err_pos;
    const char *err_msg = wst_expr_to_poly(expr, strlen(expr), &poly, &err_pos);
    if (err_msg != NULL) {
        fprintf(stderr, "Expression error at position %zu: %s", err_pos, err_msg);
        free(expr);
        return 1;
    }
    free(expr);

    WstSolution sol = wst_solve_poly(&poly);
    print_solution(&sol);
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

    /* Split program into 2 commands. */
    const char *command = argv[1];
    if (strcmp(command, "coeffs") == 0)
        return run_coeffs(argc - 1, argv + 1);
    if (strcmp(command, "expr") == 0)
        return run_expr(argc - 1, argv + 1);

    fprintf(stderr, "Unknown command '%s', run 'toys_solve --help' for usage\n", command);
    return 1;
}
