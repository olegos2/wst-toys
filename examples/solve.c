#include "toys/argparse.h"
#include "toys/debug.h"
#include "toys/solve.h"

#include <assert.h>
#include <errno.h>
#include <math.h>
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

static void print_solution(const WstSolution *sol, bool pretty)
{
    assert(sol != NULL);

    if (!pretty) {
        /* machine readable, everything on one line */
        if (sol->count == WST_SOLVE_INF)
            printf("inf\n");
        else if (sol->count == WST_SOLVE_ERR)
            printf("error\n");
        else if (sol->count == 0)
            printf("none\n");
        else
            for (int i = 0; i < sol->count; i++)
                printf("%lg%c", sol->roots[i] + 0.0,
                       i < sol->count - 1 ? ' ' : '\n');
        return;
    }

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

/* Renders coeffs as a polynomial, highest power first, e.g. x^2 - 3 x + 2 */
static void print_poly(const char *name, const double *coeffs, int degree)
{
    printf("%s(x) = ", name);

    bool first = true;
    for (int i = degree; i >= 0; i--) {
        double c = coeffs[i] + 0.0;
        if (iszero(c))
            continue;

        if (!first)
            printf(c < 0.0 ? " - " : " + ");
        else if (c < 0.0)
            printf("-");

        double a = fabs(c);
        if (i == 0 || !iszero(a - 1.0))
            printf("%lg", a);
        if (i > 0)
            printf(" x");
        if (i > 1)
            printf("^%d", i);
        first = false;
    }
    if (first)
        printf("0");
    printf("\n");
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
        print_solution(&sol, true);
    }
    free(line);
}


static void print_help_commands(void)
{
    printf(
        "Commands:\n"
        "  coeffs c0 c1 ... cn  Solve c0 + c1 x + ... + cn x^n = 0 with the\n"
        "                       given numeric coefficients\n"
        "  expr expression      Solve a mathematical expression set equal to\n"
        "                       zero, e.g. x^2 - 4 = 0 or 2 - x^2 = 0\n"
        "  deriv c0 c1 ... cn   Get coefficients of derivative of polynomial\n"
        "  (none)               interactive mode, reads coefficient lines from\n"
        "                       stdin\n");
}

/* coeffs command: solve using numeric coefficients from argv starting from argv[0]. */
static int run_coeffs(int argc, char *argv[], bool pretty)
{
    if (argc > WST_SOLVE_MAX_DEGREE + 1) {
        LOG_E("too many coefficients (max degree %d)", WST_SOLVE_MAX_DEGREE);
        return 1;
    }

    /* Parse coefficients separately, with malformed numbers checks. */
    WstPoly poly = { 0 };
    int count = 0;
    for (int i = 0; i < argc; i++) {
        if (!parse_coeff(argv[i], strlen(argv[i]), &poly.coeffs[count])) {
            LOG_E("invalid coefficient %s", argv[i]);
            return 1;
        }
        poly.degree = count;
        count++;
    }

    WstSolution sol = wst_solve_poly(&poly);
    print_solution(&sol, pretty);
    return 0;
}

/* deriv command: print derivative polynomial coefficients, constant term first. */
static int run_deriv(int argc, char *argv[], bool pretty)
{
    if (argc > WST_SOLVE_MAX_DEGREE + 1) {
        LOG_E("too many coefficients (max degree %d)", WST_SOLVE_MAX_DEGREE);
        return 1;
    }
    if (argc < 1) {
        LOG_E("no coefficients given, run 'toys_solve --help' for usage");
        return 1;
    }

    WstPoly poly = { 0 };
    int count = 0;
    for (int i = 0; i < argc; i++) {
        if (!parse_coeff(argv[i], strlen(argv[i]), &poly.coeffs[count])) {
            LOG_E("invalid coefficient %s", argv[i]);
            return 1;
        }
        poly.degree = count;
        count++;
    }

    WstPoly d = wst_poly_deriv(&poly);

    if (pretty) {
        print_poly("P'", d.coeffs, d.degree);
        return 0;
    }
    for (int i = 0; i <= d.degree; i++)
        printf("%lg%c", d.coeffs[i], i < d.degree ? ' ' : '\n');
    return 0;
}

/* TODO: expr command */
static int run_expr(int argc, char *argv[])
{
    if (argc < 1) {
        fprintf(stderr, "No expression given, run '%s --help' for usage\n", argv[0]);
        return 1;
    }

    /* Count space needed for remaining args to join in one string. */
    size_t total = 1;
    for (int i = 0; i < argc; i++)
        total += strlen(argv[i]) + 1;

    char *expr = malloc(total);
    if (expr == NULL) {
        LOG_E("Failed to allocate expression: %s", strerror(errno));
        return 1;
    }

    /* Copy args into one string */
    char *dst = expr;
    for (int i = 0; i < argc; i++) {
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
    print_solution(&sol, true);
    return 0;
}

int main(int argc, char *argv[])
{
    bool help = false;
    bool verbose = false;
    bool pretty = false;
    const char *command = NULL;
    const char *debug_filename = NULL;

    ArgOption opts[] = {
        {
            .type = ARG_SWITCH,
            .dest = &help,
            .short_name = "-h",
            .long_name = "--help",
            .description = "print this help and exit",
        },
#ifdef WST_DEBUG
        {
            .type = ARG_SWITCH,
            .dest = &verbose,
            .short_name = "-v",
            .long_name = "--verbose",
            .description = "enable verbose logging messages",
        },
#endif
        {
            .type = ARG_STRING,
            .dest = &debug_filename,
            .short_name = "-l",
            .long_name = "--logfile",
            .description = "redirect log prints to a file path",
        },
        {
            .type = ARG_SWITCH,
            .dest = &pretty,
            .short_name = "-p",
            .long_name = "--pretty",
            .description = "print output in human friendly format",
        },
        {
            .type = ARG_POSITIONAL,
            .dest = &command,
            .long_name = "command",
            .description = "One of subcommands described below",
        },
    };

    ArgParser parser = {
        .prog = argv[0],
        .opts = opts,
        .nopts = sizeof(opts) / sizeof(*opts),
        /* Capture subcommand args */
        .capture_rest = true,
    };

    if (!argparse_parse(&parser, argc, argv)) {
        fprintf(stderr, "%s, run '%s --help' for usage\n", parser.error, argv[0]);
        return 1;
    }
    if (help) {
        argparse_print_help(&parser);
        print_help_commands();
        return 0;
    }

    if (verbose)
        toys_log_set_max_prio(WST_LOG_VERBOSE);

    if (!toys_log_open(debug_filename))
        LOG_W("Failed to open log file for writing");
    LOG_D("Started logger");

    if (command == NULL) {
        run_interactive();
        return 0;
    }

    if (strcmp(command, "coeffs") == 0)
        return run_coeffs(parser.nrest, parser.rest, pretty);
    if (strcmp(command, "expr") == 0)
        return run_expr(parser.nrest, parser.rest);
    if (strcmp(command, "deriv") == 0)
        return run_deriv(parser.nrest, parser.rest, pretty);

    fprintf(stderr, "Unknown command '%s', run '%s --help' for usage\n", command, argv[0]);
    return 1;
}
