#include "toys/argparse.h"
#include "toys/debug.h"
#include "toys/solve.h"
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

typedef struct {
    bool verbose;
    bool pretty;
    bool expr_mode;
} SolveConfig;

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
        wst_poly_print(buf, sizeof(buf), "P", &poly, true);
        printf("%s\n", buf);
    }

    WstSolution sol = wst_solve_poly(&poly);
    print_solution(&sol, cfg->pretty);

    WstPoly deriv = wst_poly_deriv(&poly);
    wst_poly_print(buf, sizeof(buf), "P'", &deriv, cfg->pretty);
    printf("%s\n", buf);

    WstPoly integ = { 0 };
    if (wst_poly_integ(&poly, &integ)) {
        wst_poly_print(buf, sizeof(buf), "\\int P", &integ, cfg->pretty);
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
        "  solve       Solve polynomial for real roots, find deriv and integrate\n"
        "  plot        Plot a polynomial\n"
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

    char poly_pretty[POLY_BUF_LEN];
    wst_poly_print(poly_pretty, sizeof(poly_pretty), "y", &poly, true);
    printf("%s\n", poly_pretty);

    WstSolution sol = wst_solve_poly(&poly);
    print_solution(&sol, cfg->pretty);

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

    if (cfg.verbose)
        toys_log_set_max_prio(WST_LOG_VERBOSE);

    if (!toys_log_open(debug_filename))
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

    fprintf(stderr, "Unknown command '%s', run '%s --help' for usage\n", command, argv[0]);
    return 1;
}
