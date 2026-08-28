#include "toys/debug.h"
#include "toys/math.h"
#include "toys/poly.h"

#include <raylib.h>
#include <raymath.h>
#include <math.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_BUFFER_SIZE 4096
#define POLY_BUF_LEN 256

#define ARR_LEN(arr) (sizeof(arr) / sizeof(*arr))

typedef struct {
    Vector2 start;
    Vector2 end;
} PlotRect;

typedef struct {
    /** Position of grid relative to screen center. */
    Vector2 pos;
    /** Position of grid relative to screen center in px (derived). */
    Vector2 px_pos;
    /** Visible rect on screen in plot units (derived). */
    PlotRect rect;
    /** Size of one grid unit on screen in px. */
    Vector2 size;
    /** Target in pixels of marks step. */
    Vector2 target_marks_step;
    /** How many grid units are between two marks on an axis. */
    Vector2 marks_step;
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
    float low_freq;
    float high_freq;
} PlotAudioConfig;

typedef struct {
    PlotWindowConfig win;
    PlotGridConfig grid;
    PlotFontConfig font;
    PlotAudioConfig audio;
    /** Position of screen center (derived). */
    Vector2 center;
    float x_step;
    /** Factor - 1 by which plot is scaled on scroll. */
    float scroll_sensitivity;
    /** Distance between mouse and plot (vertically) at which mouse is considered on plot. */
    float plot_snap_px;
} PlotConfig;

typedef struct {
    float freq;
    float target_freq;
    float phase;
    float interp_factor;
} AudioSynth;

static inline float round_down(float a, float mod)
{
    return a - fmodf(a, mod);
}

static float round_to_digits(float a, int digits)
{
    if (my_iszerof(a)) return 0.0;
    float factor = powf(10.0, (float)digits - ceilf(log10f(fabsf(a))));
    return roundf(a * factor) / factor;   
}


/** Derives some fields. */
static void rebuild_plot_config(PlotConfig *cfg)
{
    cfg->grid.px_pos.x = cfg->grid.pos.x * cfg->grid.size.x;
    cfg->grid.px_pos.y = cfg->grid.pos.y * cfg->grid.size.y;

    cfg->center.x = (float)cfg->win.w / 2.f;
    cfg->center.y = (float)cfg->win.h / 2.f;
    cfg->x_step = 1.f / cfg->grid.size.x;

    float len_x = (float)cfg->win.w / cfg->grid.size.x;
    float len_y = (float)cfg->win.h / cfg->grid.size.y;

    cfg->grid.rect.start.x = -len_x / 2.f + cfg->grid.pos.x;
    cfg->grid.rect.start.y = -len_y / 2.f + cfg->grid.pos.y;
    cfg->grid.rect.end.x = cfg->grid.rect.start.x + len_x;
    cfg->grid.rect.end.y = cfg->grid.rect.start.y + len_y;

    cfg->grid.marks_step.x = cfg->grid.target_marks_step.x / cfg->grid.size.x;
    cfg->grid.marks_step.y = cfg->grid.target_marks_step.y / cfg->grid.size.y;

    cfg->grid.marks_step.x = round_to_digits(cfg->grid.marks_step.x, 2);
    cfg->grid.marks_step.y = round_to_digits(cfg->grid.marks_step.y, 2);

    LOG_V("w %d, h %d, pos %f %f, start %f %f, end %f %f, "
          "size %f %f, marks_step %f %f, freq %f %f, "
          "center %f %f", cfg->win.w, cfg->win.h, cfg->grid.pos.x, cfg->grid.pos.y,
          cfg->grid.rect.start.x, cfg->grid.rect.start.y, cfg->grid.rect.end.x, cfg->grid.rect.end.y,
          cfg->grid.size.x, cfg->grid.size.y, cfg->grid.marks_step.x, cfg->grid.marks_step.y,
          cfg->audio.low_freq, cfg->audio.high_freq, cfg->center.x, cfg->center.y);
}

static void move_plot(PlotConfig *cfg, Vector2 new_pos)
{
    cfg->grid.pos = new_pos;
    rebuild_plot_config(cfg);
}

static inline Vector2 scale_pos(const PlotConfig *cfg, Vector2 pos)
{
    return (Vector2){ .x = pos.x * cfg->grid.size.x, .y = -pos.y * cfg->grid.size.y };
}

static inline Vector2 abs_to_relat(const PlotConfig *cfg, Vector2 abs_pos)
{
    return (Vector2){ .x = abs_pos.x - cfg->grid.px_pos.x, .y = abs_pos.y + cfg->grid.px_pos.y };
}

static inline Vector2 relat_to_screen(const PlotConfig *cfg, Vector2 rel_pos)
{
    return (Vector2){ .x = rel_pos.x + cfg->center.x, .y = rel_pos.y + cfg->center.y };
}

static inline Vector2 abs_to_screen(const PlotConfig *cfg, Vector2 abs_pos)
{
    return relat_to_screen(cfg, abs_to_relat(cfg, abs_pos));
}

static inline Vector2 pos_to_screen(const PlotConfig *cfg, Vector2 pos)
{
    return abs_to_screen(cfg, scale_pos(cfg, pos));
}

static inline Vector2 screen_to_pos(const PlotConfig *cfg, Vector2 pos)
{
    return (Vector2){
        .x = (pos.x - cfg->center.x) / cfg->grid.size.x + cfg->grid.pos.x,
        .y = -(pos.y - cfg->center.y) / cfg->grid.size.y + cfg->grid.pos.y,
    };
}

static inline bool point_rect_intersection(PlotRect rect, Vector2 p)
{
    return p.x >= rect.start.x && p.x <= rect.end.x && p.y >= rect.start.y && p.y <= rect.end.y;
}

static void draw_background_grid(const PlotConfig *cfg)
{
    assert(cfg != NULL);

    Vector2 start = {
        .x = round_down(cfg->grid.rect.start.x, cfg->grid.marks_step.x),
        .y = round_down(cfg->grid.rect.start.y, cfg->grid.marks_step.y),
    };

    for (float i = start.x; i <= cfg->grid.rect.end.x; i += cfg->grid.marks_step.x) {
        Vector2 p = pos_to_screen(cfg, (Vector2){ .x = i, .y = 0.f });
        DrawLineEx((Vector2){ .x = p.x, .y = 0 },
                   (Vector2){ .x = p.x, .y = (float)cfg->win.w },
                   1.f, DARKGRAY);
    }
    for (float i = start.y; i <= cfg->grid.rect.end.y; i += cfg->grid.marks_step.y) {
        Vector2 p = pos_to_screen(cfg, (Vector2){ .x = 0.f, .y = i });
        DrawLineEx((Vector2){ .x = 0, .y = p.y },
                   (Vector2){ .x = (float)cfg->win.w, .y = p.y },
                   1.f, DARKGRAY);
    }
}

static void draw_main_axes(const PlotConfig *cfg)
{
    assert(cfg != NULL);

    Vector2 pos = abs_to_screen(cfg, (Vector2){ 0 });

    DrawLineEx((Vector2){ .x = pos.x, .y = 0 },
               (Vector2){ .x = pos.x, .y = (float)cfg->win.h },
               1.f, LIGHTGRAY);
    DrawLineEx((Vector2){ .x = 0, .y = pos.y },
               (Vector2){ .x = (float)cfg->win.w, .y = pos.y },
               1.f, LIGHTGRAY);
}

static void draw_axes_number_lines(const PlotConfig *cfg)
{
    assert(cfg != NULL);

    const char *text = NULL;
    int text_width = 0;
    Vector2 p;

    Vector2 start = {
        .x = round_down(cfg->grid.rect.start.x, cfg->grid.marks_step.x),
        .y = round_down(cfg->grid.rect.start.y, cfg->grid.marks_step.y),
    };

    Vector2 origin = {
        .x = cfg->grid.marks_step.x / 2.f,
        .y = cfg->grid.marks_step.y / 2.f,
    };

    // Draw axis x number line (except 0)
    for (float i = start.x; i <= cfg->grid.rect.end.x; i += cfg->grid.marks_step.x) {
        if (-origin.x < i && i < origin.x)
            continue;
        text = TextFormat("%.4g", i);
        text_width = MeasureText(text, cfg->font.label);
        p = pos_to_screen(cfg, (Vector2){ .x = i, .y = 0.f });
        DrawText(text, (int)p.x - text_width / 2,
                 (int)p.y - 4 - cfg->font.label, cfg->font.label, WHITE);
    }
    // Draw axis y number line (except 0)
    for (float i = start.y; i <= cfg->grid.rect.end.y; i += cfg->grid.marks_step.y) {
        if (-origin.y < i && i < origin.y)
            continue;
        p = pos_to_screen(cfg, (Vector2){ .x = 0.f, .y = i });
        DrawText(TextFormat("%.4g", i), (int)p.x + 4,
                 (int)p.y - cfg->font.label / 2,
                 cfg->font.label, WHITE);
    }
    // Draw zero
    p = pos_to_screen(cfg, (Vector2){ 0 });
    DrawText("0", (int)p.x + 4, (int)p.y - cfg->font.label - 4,
             cfg->font.label, WHITE);
}

static void draw_axes_labels(const PlotConfig *cfg)
{
    assert(cfg != NULL);

    const char *text = NULL;
    int text_width = 0;
    Vector2 p;

    text = "x";
    text_width = MeasureText(text, cfg->font.title);
    p = pos_to_screen(cfg, (Vector2){ .x = cfg->grid.rect.end.x, .y = 0.f });
    DrawText(text, (int)p.x - text_width - 4,
             (int)p.y + 2, cfg->font.title, GREEN);

    text = "y";
    text_width = MeasureText(text, cfg->font.title);
    p = pos_to_screen(cfg, (Vector2){ .x = 0.f, .y = cfg->grid.rect.end.y });
    DrawText(text, (int)p.x - text_width - 4,
             (int)p.y + 2, cfg->font.title, GREEN);
}

static void draw_poly_plot(const PlotConfig *cfg, const WstPoly *poly)
{
    assert(cfg != NULL);
    assert(poly != NULL);

    Vector2 saved = pos_to_screen(cfg, (Vector2){
        .x = cfg->grid.rect.start.x,
        .y = (float)wst_poly_eval(poly, cfg->grid.rect.start.x),
    });

    for (float i = (float)cfg->grid.rect.start.x + cfg->x_step; i <= cfg->grid.rect.end.x; i += cfg->x_step) {
        Vector2 now = pos_to_screen(cfg, (Vector2){ .x = i, .y = (float)wst_poly_eval(poly, i) });
        DrawLineEx(saved, now, 2.f, ORANGE);
        saved = now;
    }
}

static void draw_plot_roots(const PlotConfig *cfg, const WstSolution *sol)
{
    assert(cfg != NULL);
    assert(sol != NULL);

    for (int i = 0; i < sol->count; i++) {
        Vector2 p = pos_to_screen(cfg, (Vector2){ .x = (float)sol->roots[i], .y = 0.f });

        DrawCircle((int)p.x, (int)p.y, 4.0, ORANGE);
        DrawText(TextFormat("%.2lg", sol->roots[i]),
                 (int)p.x, (int)p.y + 2,
                 cfg->font.title, GREEN);
    }
}

static void draw_plot_deriv(const PlotConfig *cfg, const WstPoly *poly,
                            const WstPoly *deriv, Vector2 mouse_pos)
{
    Vector2 mouse_grid_pos = screen_to_pos(cfg, mouse_pos);

    float min_y = +INFINITY;
    float max_y = -INFINITY;
    float start_x = mouse_grid_pos.x - cfg->x_step * 6.0f;
    float end_x = mouse_grid_pos.x + cfg->x_step * 6.5f;

    for (float i = start_x; i <= end_x; i += cfg->x_step) {
        float y = (float)wst_poly_eval(poly, i);
        if (y > max_y) max_y = y;
        if (y < min_y) min_y = y;
    }

    Vector2 plot_point = { .x = mouse_grid_pos.x, .y = (float)wst_poly_eval(poly, mouse_grid_pos.x) };
    Vector2 screen_point = pos_to_screen(cfg, plot_point);
    Vector2 jitter = { .x = cfg->plot_snap_px / cfg->grid.size.x, .y = cfg->plot_snap_px / cfg->grid.size.y };

    // LOG_V("min_y %f, max_y %f, start_x %f, end_x %f, mouse %f %f, jitter %f %f",
    //       min_y, max_y, start_x, end_x, mouse_grid_pos.x, mouse_grid_pos.y, jitter.x, jitter.y);

    if (!point_rect_intersection((PlotRect){
        .start = { .x = start_x - jitter.x, .y = min_y - jitter.y },
        .end = { .x = end_x + jitter.x, .y = max_y + jitter.y },
    }, mouse_grid_pos))
        return;

    DrawCircle((int)screen_point.x, (int)screen_point.y, 4.f, YELLOW);

    // y'(x)  =  f(x_0) + f'(x_0) * (x - x_0)  =  f(x_0) - x_0 * f'(x_0) + f'(x_0) * x
    float deriv_slope = (float)wst_poly_eval(deriv, plot_point.x);
    WstPoly tangent = { .degree = 1, .coeffs = { plot_point.y - plot_point.x * deriv_slope, deriv_slope } };
    Vector2 tangent_start = { .x = cfg->grid.rect.start.x };
    Vector2 tangent_end = { .x = cfg->grid.rect.end.x };
    tangent_start.y = (float)wst_poly_eval(&tangent, tangent_start.x);
    tangent_end.y = (float)wst_poly_eval(&tangent, tangent_end.x);

    DrawLineEx(pos_to_screen(cfg, tangent_start),
               pos_to_screen(cfg, tangent_end), 1.f, WHITE);

    char tangent_str[POLY_BUF_LEN];
    wst_poly_print(tangent_str, ARR_LEN(tangent_str), "y'", &tangent, true);
    DrawText(tangent_str, (int)screen_point.x + 12, (int)screen_point.y - cfg->font.title - 12,
             cfg->font.title, YELLOW);
}

static AudioSynth audio_synth = {
    .interp_factor = 0.005f,
}, *synth = &audio_synth;

static void plot_audio_callback(void *frames_out, unsigned int frame_count)
{
    float *buf = (float *)frames_out;
    
    for (unsigned int i = 0; i < frame_count; i++) {
        synth->freq += (synth->target_freq - synth->freq) * synth->interp_factor;
        buf[i] = sinf(synth->phase);
        synth->phase = fmodf(synth->phase + (2.f * PI * synth->freq) / AUDIO_SAMPLE_RATE, 2.f * PI);
    }
}

int solve_run_plot(const WstPoly *poly, const WstSolution *sol)
{
    char poly_pretty[POLY_BUF_LEN];
    wst_poly_print(poly_pretty, POLY_BUF_LEN, "y", poly, true);
    printf("%s\n", poly_pretty);

    WstPoly deriv = wst_poly_deriv(poly);
    // wst_poly_print(buf, ARR_LEN(buf), "P'", &deriv, cfg->pretty);
    // printf("%s\n", buf);

    SetTargetFPS(60);

    PlotConfig cfg = {
        .win = {
            .w = 800,
            .h = 600,
        },
        .grid = {
            .size = { .x = 40.f, .y = 40.f },
            .target_marks_step = { .x = 80.f, .y = 80.f },
        },
        .audio = {
            .low_freq = 150.f,
            .high_freq = 2000.f,
        },
        .font = {
            .title = 22,
            .label = 16,
        },
        .scroll_sensitivity = 0.2f,
        .plot_snap_px = 12.f,
    };

    move_plot(&cfg, (Vector2){ 0 });
    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(cfg.win.w, cfg.win.h, "Polynomial plot");

    InitAudioDevice();

    SetAudioStreamBufferSizeDefault(AUDIO_BUFFER_SIZE);
    AudioStream stream = LoadAudioStream(AUDIO_SAMPLE_RATE, 32, 1);

    SetAudioStreamCallback(stream, plot_audio_callback);
    PlayAudioStream(stream);

    float sound_x = cfg.grid.rect.start.x;
    bool playing_sound = false;

    Vector2 pan_start = { 0 };
    Vector2 saved_grid_pos = { 0 };
    bool panning = false;

    while (!WindowShouldClose()) {
        if (IsWindowResized()) {
            cfg.win.w = GetScreenWidth();
            cfg.win.h = GetScreenHeight();
            rebuild_plot_config(&cfg);
        }

        if (IsKeyPressed(KEY_SPACE)) {
            sound_x = cfg.grid.rect.start.x;
            playing_sound = true;
        }

        Vector2 mouse_pos = GetMousePosition();

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            pan_start = mouse_pos;
            saved_grid_pos = cfg.grid.pos;
            panning = true;
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            panning = false;

        if (panning) {
            Vector2 pan_delta = { .x = mouse_pos.x - pan_start.x, .y = mouse_pos.y - pan_start.y };
            move_plot(&cfg, (Vector2){
                .x = saved_grid_pos.x - pan_delta.x / cfg.grid.size.x,
                .y = saved_grid_pos.y + pan_delta.y / cfg.grid.size.y,
            });
        }

        float scroll = GetMouseWheelMove();
        if (!my_iszerof(scroll)) {
            float scale = powf(cfg.scroll_sensitivity + 1.f, scroll);
            cfg.grid.size.x = cfg.grid.size.x * scale;
            if (!IsKeyDown(KEY_LEFT_SHIFT))
                cfg.grid.size.y = cfg.grid.size.y * scale;
            rebuild_plot_config(&cfg);
        }

        if (playing_sound) {
            sound_x += 0.03f;
            if (sound_x > cfg.grid.rect.end.x)
                playing_sound = false;

            float sound_y = (float)wst_poly_eval(poly, sound_x);
            float sound_y_lin = (sound_y - cfg.grid.rect.start.y) / (cfg.grid.rect.end.y - cfg.grid.rect.start.y);
            if (sound_y_lin > 2.f) sound_y_lin = 2.f;
            else if (sound_y_lin < -1.f) sound_y_lin = -1.f;
            synth->target_freq = cfg.audio.low_freq * powf(cfg.audio.high_freq / cfg.audio.low_freq, sound_y_lin);
        }

        BeginDrawing();
            ClearBackground(BLACK);

            draw_background_grid(&cfg);
            draw_main_axes(&cfg);
            draw_axes_number_lines(&cfg);
            draw_axes_labels(&cfg);
            draw_poly_plot(&cfg, poly);
            draw_plot_roots(&cfg, sol);

            // Draw polynomial expression
            DrawText(TextFormat("%s", poly_pretty),
                     10, 10, cfg.font.title, YELLOW);

            if (playing_sound)
                DrawLineEx((Vector2){ .x = cfg.center.x + sound_x * (float)cfg.grid.size.x, .y = 0, },
                           (Vector2){ .x = cfg.center.x + sound_x * (float)cfg.grid.size.x, .y = (float)cfg.win.h },
                           1.f, WHITE);

            draw_plot_deriv(&cfg, poly, &deriv, mouse_pos);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
