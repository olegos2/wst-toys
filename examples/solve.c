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

typedef struct {
    /** Visible rect on screen of plot in plot units. */
    float start_x;
    float end_x;
    float start_y;
    float end_y;
    /** Size of one grid unix on screen in px. */
    int size;
} PlotGridConfig;

typedef struct {
    int w;
    int h;
} PlotWindowConfig;

typedef struct {
    int label;
    int title;
} PlotFontConfig;

typedef struct {
    PlotWindowConfig win;
    PlotGridConfig grid;
    PlotFontConfig font;
    /** Position of plot origin on screen. */
    float center_x;
    float center_y;
    float x_step;
} PlotConfig;

static void draw_background_grid(const PlotConfig *cfg)
{
    assert(cfg != NULL);

    // start should be ceil'ed I think
    for (int i = (int)cfg->grid.start_x; i <= (int)cfg->grid.end_x; i++) {
        DrawLineEx((Vector2){ .x = cfg->center_x + (float)(i * cfg->grid.size), .y = 0 },
                   (Vector2){ .x = cfg->center_x + (float)(i * cfg->grid.size), .y = (float)cfg->win.w },
                   1.0, DARKGRAY);
    }
    for (int i = (int)cfg->grid.start_y; i <= (int)cfg->grid.end_y; i++) {
        DrawLineEx((Vector2){ .x = 0, .y = cfg->center_y + (float)(i * cfg->grid.size) },
                   (Vector2){ .x = (float)cfg->win.w, .y = cfg->center_y + (float)(i * cfg->grid.size) },
                   1.0, DARKGRAY);
    }
}

static void draw_main_axes(const PlotConfig *cfg)
{
    assert(cfg != NULL);

    DrawLineEx((Vector2){ .x = cfg->center_x, .y = 0 },
               (Vector2){ .x = cfg->center_x, .y = (float)cfg->win.h },
               1.0, GRAY);
    DrawLineEx((Vector2){ .x = 0, .y = cfg->center_y },
               (Vector2){ .x = (float)cfg->win.w, .y = cfg->center_y },
               1.0, GRAY);
}

static void draw_axes_number_lines(const PlotConfig *cfg)
{
    assert(cfg != NULL);

    const char *text = NULL;
    int text_width = 0;

    // Draw axis x number line (except 0)
    for (int i = (int)cfg->grid.start_x; i <= (int)cfg->grid.end_x; i++) {
        if (i == 0) continue;
        text = TextFormat("%d", i);
        text_width = MeasureText(text, cfg->font.label);
        DrawText(text, (int)cfg->center_x + i * cfg->grid.size - text_width / 2,
                 (int)cfg->center_y - 4 - cfg->font.label, cfg->font.label, WHITE);
    }
    // Draw axis y number line (except 0)
    for (int i = (int)cfg->grid.start_y; i <= (int)cfg->grid.end_y; i++) {
        if (i == 0) continue;
        /* Flip vertically */
        DrawText(TextFormat("%d", i), (int)cfg->center_x + 4,
                 (int)cfg->center_y - i * cfg->grid.size - cfg->font.label / 2,
                 cfg->font.label, WHITE);
    }
    // Draw zero
    DrawText("0", (int)cfg->center_x + 4, (int)cfg->center_y - cfg->font.label - 4,
             cfg->font.label, WHITE);
}

static void draw_axes_labels(const PlotConfig *cfg)
{
    assert(cfg != NULL);

    const char *text = NULL;
    int text_width = 0;

    // Draw axis x label
    text = "x";
    text_width = MeasureText(text, cfg->font.title);
    DrawText(text, (int)(cfg->center_x + cfg->grid.end_x * (float)cfg->grid.size) - text_width - 4,
             (int)cfg->center_y + 2, cfg->font.title, GREEN);

    // Draw axis y label
    text = "y";
    text_width = MeasureText(text, cfg->font.title);
    DrawText(text, (int)cfg->center_x - text_width - 2, 4, cfg->font.title, GREEN);
}

static void draw_poly_plot(const PlotConfig *cfg, const WstPoly *poly)
{
    assert(cfg != NULL);
    assert(poly != NULL);

    // Get first polynomial point on very left
    float saved_i = cfg->grid.start_x;
    float saved_j = (float)wst_poly_eval(poly, cfg->grid.start_x);

    for (float i = (float)cfg->grid.start_x + cfg->x_step; i <= cfg->grid.end_x; i += cfg->x_step) {
        // Draw polynomial
        float j = (float)wst_poly_eval(poly, i);
        DrawLineEx(
            (Vector2){
                .x = cfg->center_x + saved_i * (float)cfg->grid.size,
                .y = cfg->center_y - saved_j * (float)cfg->grid.size
            },
            (Vector2){
                .x = cfg->center_x + i * (float)cfg->grid.size,
                .y = cfg->center_y - j * (float)cfg->grid.size
            },
            2.0, ORANGE
        );
        saved_i = i;
        saved_j = j;
    }
}

static void draw_plot_roots(const PlotConfig *cfg, const WstSolution *sol)
{
    assert(cfg != NULL);
    assert(sol != NULL);

    for (int i = 0; i < sol->count; i++) {
        // Draw roots on x axis
        int root_x = (int)(cfg->center_x + (float)sol->roots[i] * (float)cfg->grid.size);
        DrawCircle(root_x, (int)cfg->center_y, 4.0, ORANGE);
        DrawText(TextFormat("%.2lg", sol->roots[i]),
                 root_x, (int)cfg->center_y + 2,
                 cfg->font.title, GREEN);
    }
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

    PlotConfig plot_cfg = {
        .win = {
            .w = 800,
            .h = 600,
        },
        .grid = {
            .size = 40,
        },
        .font = {
            .title = 22,
            .label = 16,
        },
    };

    plot_cfg.center_x = (float)plot_cfg.win.w / 2.f;
    plot_cfg.center_y = (float)plot_cfg.win.h / 2.f;
    plot_cfg.x_step = 2.f / (float)plot_cfg.grid.size;

    float len_x = (float)plot_cfg.win.w / (float)plot_cfg.grid.size;
    float len_y = (float)plot_cfg.win.h / (float)plot_cfg.grid.size;
    InitWindow(plot_cfg.win.w, plot_cfg.win.h, "Polynomial plot");

    plot_cfg.grid.start_x = -len_x / 2;
    plot_cfg.grid.end_x = plot_cfg.grid.start_x + len_x;
    plot_cfg.grid.start_y = -len_y / 2;
    plot_cfg.grid.end_y = plot_cfg.grid.start_y + len_y;

    while (!WindowShouldClose()) {

        BeginDrawing();
            ClearBackground(BLACK);

            draw_background_grid(&plot_cfg);
            draw_main_axes(&plot_cfg);
            draw_axes_number_lines(&plot_cfg);
            draw_axes_labels(&plot_cfg);
            draw_poly_plot(&plot_cfg, &poly);
            draw_plot_roots(&plot_cfg, &sol);

            // Draw polynomial expression
            DrawText(TextFormat("%s", poly_pretty),
                     10, 10, plot_cfg.font.title, YELLOW);
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
