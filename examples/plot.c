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

typedef struct {
    /** Position of grid relative to screen center. */
    Vector2 pos;
    /** Position of grid relative to screen center in px (derived). */
    Vector2 px_pos;
    /** Visible rect top left of on screen in plot units (derived). */
    Vector2 start;
    /** Visible rect bottom right on screen in plot units (derived). */
    Vector2 end;
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
    float scroll_sensitivity;
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

    cfg->grid.start.x = -len_x / 2.f + cfg->grid.pos.x;
    cfg->grid.start.y = -len_y / 2.f + cfg->grid.pos.y;
    cfg->grid.end.x = cfg->grid.start.x + len_x;
    cfg->grid.end.y = cfg->grid.start.y + len_y;

    cfg->grid.marks_step.x = cfg->grid.target_marks_step.x / cfg->grid.size.x;
    cfg->grid.marks_step.y = cfg->grid.target_marks_step.y / cfg->grid.size.y;

    cfg->grid.marks_step.x = round_to_digits(cfg->grid.marks_step.x, 2);
    cfg->grid.marks_step.y = round_to_digits(cfg->grid.marks_step.y, 2);

    LOG_V("w %d, h %d, pos %f %f, start %f %f, end %f %f, size %f %f, marks_step %f %f, freq %f %f "
          "center %f %f", cfg->win.w, cfg->win.h, cfg->grid.pos.x, cfg->grid.pos.y, cfg->grid.start.x,
          cfg->grid.start.y, cfg->grid.end.x, cfg->grid.end.y, cfg->grid.size.x, cfg->grid.size.y,
          cfg->grid.marks_step.x, cfg->grid.marks_step.y, cfg->audio.low_freq, cfg->audio.high_freq,
          cfg->center.x, cfg->center.y);
}

static void move_plot(PlotConfig *cfg, Vector2 new_pos)
{
    cfg->grid.pos = new_pos;
    rebuild_plot_config(cfg);
}

static inline Vector2 scale_pos(const PlotConfig *cfg, Vector2 pos)
{
    return (Vector2){ .x = pos.x * cfg->grid.size.x, .y = pos.y * cfg->grid.size.y };
}

static inline Vector2 abs_to_relat(const PlotConfig *cfg, Vector2 abs_pos)
{
    return (Vector2){ .x = abs_pos.x - cfg->grid.px_pos.x, .y = abs_pos.y - cfg->grid.px_pos.y };
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

static void draw_background_grid(const PlotConfig *cfg)
{
    assert(cfg != NULL);

    Vector2 start = {
        .x = round_down(cfg->grid.start.x, cfg->grid.marks_step.x),
        .y = round_down(cfg->grid.start.y, cfg->grid.marks_step.y),
    };

    for (float i = start.x; i <= cfg->grid.end.x; i += cfg->grid.marks_step.x) {
        float x = cfg->center.x + (i - cfg->grid.pos.x) * cfg->grid.size.x;
        DrawLineEx((Vector2){ .x = x, .y = 0 },
                   (Vector2){ .x = x, .y = (float)cfg->win.w },
                   1.f, DARKGRAY);
    }
    for (float i = start.y; i <= cfg->grid.end.y; i += cfg->grid.marks_step.y) {
        float y = cfg->center.y + (i - cfg->grid.pos.y) * cfg->grid.size.y;
        DrawLineEx((Vector2){ .x = 0, .y = y },
                   (Vector2){ .x = (float)cfg->win.w, .y = y },
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
        .x = round_down(cfg->grid.start.x, cfg->grid.marks_step.x),
        .y = round_down(cfg->grid.start.y, cfg->grid.marks_step.y),
    };

    Vector2 origin = {
        .x = cfg->grid.marks_step.x / 2.f,
        .y = cfg->grid.marks_step.y / 2.f,
    };

    // Draw axis x number line (except 0)
    for (float i = start.x; i <= cfg->grid.end.x; i += cfg->grid.marks_step.x) {
        if (-origin.x < i && i < origin.x)
            continue;
        text = TextFormat("%.4g", i);
        text_width = MeasureText(text, cfg->font.label);
        p = pos_to_screen(cfg, (Vector2){ .x = i, .y = 0.f });
        DrawText(text, (int)p.x - text_width / 2,
                 (int)p.y - 4 - cfg->font.label, cfg->font.label, WHITE);
    }
    // Draw axis y number line (except 0)
    for (float i = start.y; i <= cfg->grid.end.y; i += cfg->grid.marks_step.y) {
        if (-origin.y < i && i < origin.y)
            continue;
        p = pos_to_screen(cfg, (Vector2){ .x = 0.f, .y = i });
        /* Flip vertically */
        DrawText(TextFormat("%.4g", -i), (int)p.x + 4,
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
    p = pos_to_screen(cfg, (Vector2){ .x = cfg->grid.end.x, .y = 0.f });
    DrawText(text, (int)p.x - text_width - 4,
             (int)p.y + 2, cfg->font.title, GREEN);

    text = "y";
    text_width = MeasureText(text, cfg->font.title);
    p = pos_to_screen(cfg, (Vector2){ .x = 0.f, .y = cfg->grid.start.y });
    DrawText(text, (int)p.x - text_width - 4,
             (int)p.y + 2, cfg->font.title, GREEN);
}

static void draw_poly_plot(const PlotConfig *cfg, const WstPoly *poly)
{
    assert(cfg != NULL);
    assert(poly != NULL);

    Vector2 saved = pos_to_screen(cfg, (Vector2){
        .x = cfg->grid.start.x,
        .y = -(float)wst_poly_eval(poly, cfg->grid.start.x),
    });

    for (float i = (float)cfg->grid.start.x + cfg->x_step; i <= cfg->grid.end.x; i += cfg->x_step) {
        Vector2 now = pos_to_screen(cfg, (Vector2){ .x = i, .y = -(float)wst_poly_eval(poly, i) });
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
    };

    move_plot(&cfg, (Vector2){ 0 });
    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(cfg.win.w, cfg.win.h, "Polynomial plot");

    InitAudioDevice();

    SetAudioStreamBufferSizeDefault(AUDIO_BUFFER_SIZE);
    AudioStream stream = LoadAudioStream(AUDIO_SAMPLE_RATE, 32, 1);

    SetAudioStreamCallback(stream, plot_audio_callback);
    PlayAudioStream(stream);

    float sound_x = cfg.grid.start.x;
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
            sound_x = cfg.grid.start.x;
            playing_sound = true;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            pan_start = GetMousePosition();
            saved_grid_pos = cfg.grid.pos;
            panning = true;
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            panning = false;

        if (panning) {
            Vector2 pan_now = GetMousePosition();
            Vector2 pan_delta = { .x = pan_now.x - pan_start.x, .y = pan_now.y - pan_start.y };
            move_plot(&cfg, (Vector2){
                .x = saved_grid_pos.x - pan_delta.x / cfg.grid.size.x,
                .y = saved_grid_pos.y - pan_delta.y / cfg.grid.size.y,
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
            if (sound_x > cfg.grid.end.x)
                playing_sound = false;

            float sound_y = (float)wst_poly_eval(poly, sound_x);
            float sound_y_lin = (sound_y - cfg.grid.start.y) / (cfg.grid.end.y - cfg.grid.start.y);
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
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
