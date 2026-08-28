#include "toys/argparse.h"
#include "toys/debug.h"
#include "toys/poly.h"
#include "toys/expr.h"
#include "toys/math.h"

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <raylib.h>
#include <raymath.h>

#if defined(_MSC_VER) || defined(_WIN32)
#  include <io.h>
#  define isatty _isatty
#  define fileno _fileno
#else
#  include <unistd.h>
#endif

#define POLY_BUF_LEN 256
#define ARR_LEN(arr) (sizeof(arr) / sizeof(*arr))

typedef struct {
    bool verbose;
    bool pretty;
    bool expr_mode;
} SolveConfig;

int solve_run_plot(const WstPoly *poly, const WstSolution *sol);

static double round_to_zero(double a)
{
    return my_iszero(a) ? 0.0 : a;
}

static void print_solution(const WstSolution *sol, bool pretty)
{
    assert(sol != NULL);

    if (!pretty) {
        if (sol->count == WST_SOLVE_INF)
            printf("inf\n");
        else if (sol->count == WST_SOLVE_ERR)
            printf("error\n");
        else if (sol->count == 0)
            printf("none\n");
        else
            for (int i = 0; i < sol->count; i++)
                printf("%lg%c", sol->roots[i],
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
    for (int i = 0; i < sol->count; i++) {
        printf("x%d = %lg\n", i + 1, round_to_zero(sol->roots[i]));
    }
}

/** Does not free `expr` if it was dynamically allocated. */
static bool analyze_expr(const char *expr, SolveConfig *cfg)
{
    WstPoly poly;
    size_t err_pos;
    WstParserErr err_msg = wst_expr_to_poly(expr, &poly, &err_pos, cfg->expr_mode);
    if (err_msg != WST_EXPR_NO_ERR) {
        /* TODO: add more visual error position pointing. */
        fprintf(stderr, "Expression error at position %zu: %s",
                err_pos, wst_expr_err_string(err_msg));
        return false;
    }

    char buf[POLY_BUF_LEN];

    if (cfg->pretty) {
        wst_poly_print(buf, ARR_LEN(buf), "P", &poly, true);
        printf("%s\n", buf);
    }

    WstSolution sol = wst_poly_solve(&poly);
    print_solution(&sol, cfg->pretty);

    WstPoly deriv = wst_poly_deriv(&poly);
    wst_poly_print(buf, ARR_LEN(buf), "P'", &deriv, cfg->pretty);
    printf("%s\n", buf);

    WstPoly integ = { 0 };
    if (wst_poly_integ(&poly, &integ)) {
        wst_poly_print(buf, ARR_LEN(buf), "\\int P", &integ, cfg->pretty);
        printf("%s\n", buf);
    }

    return true;
}

/** Allocates string to hold expression dynamically, must be freed by user. */
static char *concat_args(int argc, char *argv[])
{
    /* Count space needed for remaining args to join in one string. */
    size_t total = 1;
    for (int i = 0; i < argc; i++)
        total += strlen(argv[i]) + 1;

    char *expr = malloc(total);
    if (expr == NULL) {
        LOG_E("Failed to allocate expression: %s", strerror(errno));
        return NULL;
    }

    /* Copy args into one string */
    char *dst = expr;
    for (int i = 0; i < argc; i++) {
        if (i > 0)
            *dst++ = ' ';
        size_t n = strlen(argv[i]);
        memcpy(dst, argv[i], n);
        dst += n;
    }
    *dst = '\0';

    return expr;
}

static void run_interactive(SolveConfig *cfg)
{
    if (isatty(fileno(stdin))) {
        printf("Analyze polynomials and expressions for real roots, derivative and integral.\n");
        if (cfg->expr_mode) {
            printf("Type the coefficients from the constant term up to the highest power of x,\n");
            printf("separated by spaces, then press Enter. Example:\n");
            printf("  2 -3 1  solves  x^2 - 3 x + 2 = 0\n");
        } else {
            printf("Type expression using x as variable, example:\n");
            printf("  (x + 1) ^ 2 + 5 * (-x - 1)\n");
        }
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

        analyze_expr(line, cfg);
    }
    free(line);
}

static void print_help_commands(void)
{
    printf(
        "Commands:\n"
        "  solve       Solve polynomial or expression for real roots, find deriv and integrate\n"
        "  plot        Plot a polynomial or expression\n"
        "  gen         Generate a polynomial from real roots\n"
        "  (none)      interactive mode, similar to 'solve' subcommand\n");
}

static int run_solve(int argc, char *argv[], SolveConfig *cfg)
{
    if (argc < 1) {
        fprintf(stderr, "No expression/coeffs given, use --help for usage\n");
        return 1;
    }

    char *expr = concat_args(argc, argv);
    if (expr == NULL)
        return 1;

    bool ret = analyze_expr(expr, cfg);
    free(expr);
    expr = NULL;
    return ret ? 0 : 1;
}

static int run_gen(int argc, char *argv[], SolveConfig *cfg)
{
    WstPoly poly = { 0 };
    char poly_buf[POLY_BUF_LEN];

    if (argc < 1) {
        fprintf(stderr, "No roots given? Use --help for usage\n");
        poly.degree = 0;
        poly.coeffs[0] = 1.0;
        wst_poly_print(poly_buf, ARR_LEN(poly_buf), "P", &poly, cfg->pretty);
        printf("%s\n", poly_buf);
        return 0;
    }

    char *expr = concat_args(argc, argv);
    if (expr == NULL)
        return 1;

    WstPoly roots = { 0 };
    size_t err_pos = 0;
    WstParserErr err_msg = wst_expr_to_poly(expr, &roots, &err_pos, false);
    free(expr);
    expr = NULL;

    if (err_msg != WST_EXPR_NO_ERR) {
        fprintf(stderr, "Expression error at position %zu: %s",
                err_pos, wst_expr_err_string(err_msg));
        return 1;
    }

    poly.degree = 0;
    poly.coeffs[0] = 1.f;

    for (int i = 0; i <= roots.degree; i++) {
        WstPoly rhs = { .degree = 1, .coeffs = { -roots.coeffs[i], 1.f } };
        if (!wst_poly_mul(&poly, &rhs)) {
            fprintf(stderr, "Polynomial degree capacity exceeded (%d)",
                    WST_POLY_MAX_DEGREE);
            return 1;
        }
    }

    wst_poly_print(poly_buf, ARR_LEN(poly_buf), "P", &poly, cfg->pretty);
    printf("%s\n", poly_buf);

    return 0;
}

static int run_plot(int argc, char *argv[], SolveConfig *cfg)
{
    if (argc < 1) {
        fprintf(stderr, "No expression/coeffs given, use --help for usage\n");
        return 1;
    }

    char *expr = concat_args(argc, argv);
    if (expr == NULL)
        return 1;

    WstPoly poly;
    size_t err_pos;
    WstParserErr err_msg = wst_expr_to_poly(expr, &poly, &err_pos, cfg->expr_mode);
    free(expr);
    expr = NULL;

    if (err_msg != WST_EXPR_NO_ERR) {
        fprintf(stderr, "Expression error at position %zu: %s",
                err_pos, wst_expr_err_string(err_msg));
        return 1;
    }

    WstSolution sol = wst_poly_solve(&poly);
    print_solution(&sol, cfg->pretty);

    return solve_run_plot(&poly, &sol);
}

int main(int argc, char *argv[])
{
    bool help = false;
    const char *command = NULL;
    const char *debug_filename = NULL;
    SolveConfig cfg = { 0 };

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
            .dest = &cfg.verbose,
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
            .dest = &cfg.pretty,
            .short_name = "-p",
            .long_name = "--pretty",
            .description = "print output in human friendly format",
        },
        {
            .type = ARG_SWITCH,
            .dest = &cfg.expr_mode,
            .short_name = "-e",
            .long_name = "--expr",
            .description = "use mathematical expressions as input instead of raw coeffs "
                           "(applies to all subcommands)",
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
        .nopts = ARR_LEN(opts),
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

    if (cfg.verbose)
        wst_log_set_max_prio(WST_LOG_VERBOSE);

    if (!wst_log_open(debug_filename))
        LOG_W("Failed to open log file for writing");
    LOG_D("Started logger");

    if (command == NULL) {
        run_interactive(&cfg);
        return 0;
    }

    if (strcmp(command, "solve") == 0)
        return run_solve(parser.nrest, parser.rest, &cfg);
    if (strcmp(command, "plot") == 0)
        return run_plot(parser.nrest, parser.rest, &cfg);
    if (strcmp(command, "gen") == 0)
        return run_gen(parser.nrest, parser.rest, &cfg);

    fprintf(stderr, "Unknown command '%s', run '%s --help' for usage\n", command, argv[0]);
    return 1;
}
