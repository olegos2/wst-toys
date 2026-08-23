#include "toys/argparse.h"
#include "toys/debug.h"
#include "toys/solve.h"
#include "toys/math.h"

#include <assert.h>
#include <errno.h>
#include <math.h>
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
static void print_poly(char *buf, size_t nbuf, const char *name, const WstPoly *poly)
{
    char *cur = buf;
    size_t rem = nbuf;
    int written;

#define ADD(fmt, ...) \
    do { \
        written = snprintf(cur, rem, fmt, ##__VA_ARGS__); \
        if (written > 0) { \
            size_t u_written = (size_t)written; \
            if (u_written >= rem) { \
                cur += (rem - 1); \
                rem = 1; \
            } else { \
                cur += u_written; \
                rem -= u_written; \
            } \
        } \
    } while (0)

    ADD("%s(x) = ", name);

    bool first = true;
    for (int i = poly->degree; i >= 0; i--) {
        double c = poly->coeffs[i] + 0.0;
        if (iszero(c))
            continue;

        if (!first)
            ADD(c < 0.0 ? " - " : " + ");
        else if (c < 0.0)
            ADD("-");

        double a = fabs(c);
        if (i == 0 || !iszero(a - 1.0))
            ADD("%lg", a);
        if (i > 0)
            ADD("x");
        if (i > 1)
            ADD("^%d", i);
        first = false;
    }
    ADD(first ? "0\n" : "\n");

#undef ADD
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
        "  plot c0 c1 ... cn    Plot a polynomial\n"
        "  (none)               interactive mode, reads coefficient lines from\n"
        "                       stdin\n");
}

static bool parse_poly(int argc, char *argv[], WstPoly *poly)
{
    if (argc > WST_SOLVE_MAX_DEGREE + 1) {
        LOG_E("too many coefficients (max degree %d)", WST_SOLVE_MAX_DEGREE);
        return false;
    }
    if (argc < 1) {
        LOG_E("no coefficients given, run 'toys_solve --help' for usage");
        return false;
    }

    int count = 0;
    for (int i = 0; i < argc; i++) {
        if (!parse_coeff(argv[i], strlen(argv[i]), &poly->coeffs[count])) {
            LOG_E("invalid coefficient %s", argv[i]);
            return false;
        }
        poly->degree = count;
        count++;
    }
    return true;
}

/* coeffs command: solve using numeric coefficients from argv starting from argv[0]. */
static int run_coeffs(int argc, char *argv[], bool pretty)
{
    WstPoly poly = { 0 };
    if (!parse_poly(argc, argv, &poly))
        return 1;
    WstSolution sol = wst_solve_poly(&poly);
    print_solution(&sol, pretty);
    return 0;
}

/* deriv command: print derivative polynomial coefficients, constant term first. */
static int run_deriv(int argc, char *argv[], bool pretty)
{

    WstPoly poly = { 0 };
    if (!parse_poly(argc, argv, &poly))
        return 1;
    WstPoly d = wst_poly_deriv(&poly);

    if (pretty) {
        char buf[256];
        print_poly(buf, sizeof(buf), "P'", &d);
        printf("%s", buf);
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

static int run_plot(int argc, char *argv[0])
{
    WstPoly poly = { 0 };
    if (!parse_poly(argc, argv, &poly))
        return 1;

    char poly_pretty[256];
    print_poly(poly_pretty, sizeof(poly_pretty), "y", &poly);

    WstSolution sol = wst_solve_poly(&poly);
    print_solution(&sol, true);

    SetTargetFPS(60);

    const int win_width = 800, win_height = 600;
    const int grid_size = 40;

    const int center_x = win_width / 2, center_y = win_height / 2;
    int len_x = win_width / grid_size;
    int len_y = win_height / grid_size;
    InitWindow(win_width, win_height, "Polynomial plot");

    int grid_start_x = -len_x / 2;
    int grid_end_x = grid_start_x + len_x;
    int grid_start_y = -len_y / 2;
    int grid_end_y = grid_start_y + len_y;

    const int font_size = 22;
    const int label_size = 16;

    double x_step = 2.0 / (double)grid_size;

    while (!WindowShouldClose()) {
        // TODO: comments
        const char *text = NULL;
        int text_width = 0;

        BeginDrawing();
            ClearBackground(BLACK);

            // Draw background grid
            for (int i = grid_start_x * grid_size; i <= grid_end_x * grid_size; i += grid_size) {
                DrawLineEx((Vector2){ .x = (float)(center_x + i), .y = 0 },
                            (Vector2){ .x = (float)(center_x + i), .y = (float)win_height },
                            1.0, DARKGRAY);
            }
            for (int i = grid_start_y * grid_size; i <= grid_end_y * grid_size; i += grid_size) {
                DrawLineEx((Vector2){ .x = 0, .y = (float)(center_y + i) },
                            (Vector2){ .x = (float)win_width, .y = (float)(center_y + i) },
                            1.0, DARKGRAY);
            }

            // Draw axes
            DrawLineEx((Vector2){ .x = (float)center_x, .y = 0 },
                        (Vector2){ .x = (float)center_x, .y = (float)win_height },
                        1.0, GRAY);
            DrawLineEx((Vector2){ .x = 0, .y = (float)center_y },
                        (Vector2){ .x = (float)win_width, .y = (float)center_y },
                        1.0, GRAY);
            
            // Draw axis x number line (except 0)
            for (int i = grid_start_x; i <= grid_end_x; i++) {
                if (i == 0) continue;
                text = TextFormat("%d", i);
                text_width = MeasureText(text, label_size);
                DrawText(text, center_x + i * grid_size - text_width / 2,
                            center_y - 4 - label_size, label_size, WHITE);
            }
            // Draw axis y number line (except 0)
            for (int i = grid_start_y; i <= grid_end_y; i++) {
                if (i == 0) continue;
                /* Flip vertically */
                DrawText(TextFormat("%d", i), center_x + 4, center_y - i * grid_size - label_size / 2,
                         label_size, WHITE);
            }
            // Draw zero
            DrawText("0", center_x + 4, center_y - label_size - 4,
                     label_size, WHITE);

            // Draw axis x label
            text = "x";
            text_width = MeasureText(text, font_size);
            DrawText(text, center_x + grid_end_x * grid_size - text_width - 4,
                     center_y + 2, font_size, GREEN);

            // Draw axis y label
            text = "y";
            text_width = MeasureText(text, font_size);
            DrawText(text, center_x - text_width - 2, 4, font_size, GREEN);
            
            // Get first polynomial point on very left
            double saved_i = grid_start_x;
            double saved_j = wst_poly_eval(&poly, grid_start_x);

            for (double i = grid_start_x + x_step; i <= grid_end_x; i += x_step) {
                // Draw polynomial
                double j = wst_poly_eval(&poly, i);
                DrawLineEx(
                    (Vector2){
                        .x = (float)center_x + (float)saved_i * (float)grid_size,
                        .y = (float)center_y - (float)saved_j * (float)grid_size
                    },
                    (Vector2){
                        .x = (float)center_x + (float)i * (float)grid_size,
                        .y = (float)center_y - (float)j * (float)grid_size
                    },
                    2.0, ORANGE
                );
                saved_i = i;
                saved_j = j;
            }

            for (int i = 0; i < sol.count; i++) {
                // Draw roots on x axis
                int root_x = center_x + (int)(sol.roots[i] * grid_size);
                DrawCircle(root_x, center_y, 4.0, ORANGE);
                DrawText(TextFormat("%.2lg", sol.roots[i]), root_x, center_y + 2, font_size, GREEN);
            }

            // Draw polynomial expression
            DrawText(TextFormat("%s", poly_pretty), 10, 10, font_size, YELLOW);
        EndDrawing();
    }

    CloseWindow();

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
    if (strcmp(command, "plot") == 0)
        return run_plot(parser.nrest, parser.rest);

    fprintf(stderr, "Unknown command '%s', run '%s --help' for usage\n", command, argv[0]);
    return 1;
}
