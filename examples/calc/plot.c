#include "solve.h"
#include "plot.h"

#include "toys/debug.h"
#include "toys/math.h"
#include "toys/poly.h"

#include <raylib.h>
#include <raymath.h>
#include <math.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>


static AudioSynth audio_synth = {
    .interp_factor = 0.0002f,
    .volume = 1.f / POLY_CAP,
}, *synth = &audio_synth;

static const Color poly_colors[] = {
    YELLOW, GREEN, ORANGE, BLUE
};

static inline float round_down(float a, float mod)
{
    return a - fmodf(a, mod);
}

/** Round number to have `digits` significant digits. */
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

/** Convert vector from plot units to screen (px) units. */
static inline Vector2 scale_pos(const PlotConfig *cfg, Vector2 pos)
{
    return (Vector2){ .x = pos.x * cfg->grid.size.x, .y = -pos.y * cfg->grid.size.y };
}

/** Convert absolute px position to viewport center relative position */
static inline Vector2 abs_to_relat(const PlotConfig *cfg, Vector2 abs_pos)
{
    return (Vector2){
        .x = abs_pos.x - cfg->grid.px_pos.x,
        .y = abs_pos.y + cfg->grid.px_pos.y
    };
}

/** Convert viewport center-relative coords to actual viewport coords (top-left relative) */
static inline Vector2 relat_to_screen(const PlotConfig *cfg, Vector2 rel_pos)
{
    return (Vector2){ .x = rel_pos.x + cfg->center.x, .y = rel_pos.y + cfg->center.y };
}

/** Convert absolute px position in plot space to screen position (in px). */
static inline Vector2 abs_to_screen(const PlotConfig *cfg, Vector2 abs_pos)
{
    return relat_to_screen(cfg, abs_to_relat(cfg, abs_pos));
}

/** Convert plot units space position to screen position (in px). */
static inline Vector2 pos_to_screen(const PlotConfig *cfg, Vector2 pos)
{
    return abs_to_screen(cfg, scale_pos(cfg, pos));
}

/** Convert screen position (in px) to plot position (in plot units). */
static inline Vector2 screen_to_pos(const PlotConfig *cfg, Vector2 pos)
{
    return (Vector2){
        .x = (pos.x - cfg->center.x) / cfg->grid.size.x + cfg->grid.pos.x,
        .y = -(pos.y - cfg->center.y) / cfg->grid.size.y + cfg->grid.pos.y,
    };
}

/** Check if point is on edge or inside of rect. */
static inline bool point_rect_intersection(PlotRect rect, Vector2 p)
{
    return p.x >= rect.start.x && p.x <= rect.end.x &&
           p.y >= rect.start.y && p.y <= rect.end.y;
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
        text_width = MeasureText(text, cfg->font.label.sz);
        p = pos_to_screen(cfg, (Vector2){ .x = i, .y = 0.f });
        DrawText(text, (int)p.x - text_width / 2,
                 (int)p.y - 4 - cfg->font.label.sz, cfg->font.label.sz, WHITE);
    }
    // Draw axis y number line (except 0)
    for (float i = start.y; i <= cfg->grid.rect.end.y; i += cfg->grid.marks_step.y) {
        if (-origin.y < i && i < origin.y)
            continue;
        p = pos_to_screen(cfg, (Vector2){ .x = 0.f, .y = i });
        DrawText(TextFormat("%.4g", i), (int)p.x + 4,
                 (int)p.y - cfg->font.label.sz / 2,
                 cfg->font.label.sz, WHITE);
    }
    // Draw zero
    p = pos_to_screen(cfg, (Vector2){ 0 });
    DrawText("0", (int)p.x + 4, (int)p.y - cfg->font.label.sz - 4,
             cfg->font.label.sz, WHITE);
}

static void draw_axes_labels(const PlotConfig *cfg)
{
    assert(cfg != NULL);

    const char *text = NULL;
    int text_width = 0;
    Vector2 p;

    text = "x";
    text_width = MeasureText(text, cfg->font.title.sz);
    p = pos_to_screen(cfg, (Vector2){ .x = cfg->grid.rect.end.x, .y = 0.f });
    DrawText(text, (int)p.x - text_width - 4,
             (int)p.y + 2, cfg->font.title.sz, GREEN);

    text = "y";
    text_width = MeasureText(text, cfg->font.title.sz);
    p = pos_to_screen(cfg, (Vector2){ .x = 0.f, .y = cfg->grid.rect.end.y });
    DrawText(text, (int)p.x - text_width - 4,
             (int)p.y + 2, cfg->font.title.sz, GREEN);
}

static void draw_poly_plot(const PlotConfig *cfg, const WstPoly *poly, Color color)
{
    assert(cfg != NULL);
    assert(poly != NULL);

    Vector2 saved = pos_to_screen(cfg, (Vector2){
        .x = cfg->grid.rect.start.x,
        .y = (float)wst_poly_eval(poly, cfg->grid.rect.start.x),
    });

    for (float i = (float)cfg->grid.rect.start.x + cfg->x_step; i <= cfg->grid.rect.end.x; i += cfg->x_step) {
        Vector2 now = pos_to_screen(cfg, (Vector2){ .x = i, .y = (float)wst_poly_eval(poly, i) });
        DrawLineEx(saved, now, 2.f, color);
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
        DrawText(TextFormat("%.2lg", sol->roots[i] + 0.0),
                 (int)p.x, (int)p.y + 2,
                 cfg->font.title.sz, GREEN);
    }
}

static void draw_plot_deriv(const PlotConfig *cfg, const WstPoly *poly,
                            const WstPoly *deriv, Vector2 mouse_pos, const char *name)
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
    wst_poly_print(tangent_str, ARR_LEN(tangent_str), name, &tangent, true);
    DrawText(tangent_str, (int)screen_point.x + 12, (int)screen_point.y - cfg->font.title.sz - 12,
             cfg->font.title.sz, YELLOW);
    DrawText(TextFormat("(%.3g, %.3g)", plot_point.x, plot_point.y),
             (int)screen_point.x + 12, (int)screen_point.y - 2 * cfg->font.title.sz - 12,
             cfg->font.title.sz, GREEN);
}

static void plot_audio_callback(void *frames_out, unsigned int frame_count)
{
    float *buf = (float *)frames_out;
    
    for (unsigned int i = 0; i < frame_count; i++) {
        buf[i] = 0.f;
        for (int j = 0; j < POLY_CAP; j++) {
            synth->freq[j] += (synth->target_freq[j] - synth->freq[j]) * synth->interp_factor;
            buf[i] += sinf(synth->phase[j]) * audio_synth.volume;
            synth->phase[j] = fmodf(synth->phase[j] + (2.f * PI * synth->freq[j])
                                    / AUDIO_SAMPLE_RATE, 2.f * PI);
        }
    }

    for (int i = 0; i < POLY_CAP; i++) {
        if (my_iszerof(synth->freq[i])) {
            synth->freq[i] = 0.f;
            synth->phase[i] = 0.f;
        }
    }
}

int solve_run_plot(size_t npolys, const WstPoly *polys, const WstSolution *sols)
{
    char poly_pretty[npolys][POLY_BUF_LEN];
    WstPoly derivs[npolys];
    bool playing_sound = false;
    Vector2 pan_start_screen = { 0 };
    Vector2 pan_start_pos = { 0 };
    bool panning = false;

    PlotConfig cfg = {
        .win = {
            .w = 800,
            .h = 600,
        },
        .grid = {
            .size = { .x = 40.f, .y = 40.f },
            .target_marks_step = { .x = 80.f, .y = 80.f },
            .draw_background = true,
        },
        .audio = {
            .low_freq = 250.f,
            .high_freq = 2000.f,
        },
        .font = {
            .title = {
                .sz = 22,
            },
            .label = {
                .sz = 16,
            },
        },
        .scroll_sensitivity = 0.2f,
        .plot_snap_px = 12.f,
    };

    move_plot(&cfg, (Vector2){ 0 });

    float sound_x = cfg.grid.rect.start.x;

    for (size_t i = 0; i < npolys; i++) {
        wst_poly_print(poly_pretty[i], POLY_BUF_LEN,
            TextFormat("y%zu", i), &polys[i], true);
        printf("%s\n", poly_pretty[i]);
        wst_poly_deriv(&polys[i], &derivs[i]);
    }

    SetTargetFPS(60);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(cfg.win.w, cfg.win.h, "Polynomial plot");
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(AUDIO_BUFFER_SIZE);
    AudioStream stream = LoadAudioStream(AUDIO_SAMPLE_RATE, 32, 1);
    SetAudioStreamCallback(stream, plot_audio_callback);
    PlayAudioStream(stream);

    // cfg.font.title.face = LoadFontEx("assets/consola.ttf", 32, NULL, 0);
    //  DrawTextEx(monoFont, "Hello with Consolas-like font!", (Vector2){ 50.0f, 50.0f }, 24.0f, 1.0f, DARKGRAY);

    // // Set texture filter for smooth scaling
    // SetTextureFilter(monoFont.texture, TEXTURE_FILTER_BILINEAR);

    while (!WindowShouldClose()) {
        if (IsWindowResized()) {
            cfg.win.w = GetScreenWidth();
            cfg.win.h = GetScreenHeight();
            rebuild_plot_config(&cfg);
        }

        if (IsKeyPressed(KEY_SPACE)) {
            sound_x = cfg.grid.rect.start.x;
            playing_sound = !playing_sound;
        }

        if (IsKeyPressed(KEY_G)) {
            cfg.grid.draw_background = !cfg.grid.draw_background;
        }

        Vector2 mouse_pos = GetMousePosition();

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            pan_start_screen = mouse_pos;
            pan_start_pos = cfg.grid.pos;
            panning = true;
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            panning = false;

        if (panning) {
            Vector2 pan_delta = { .x = mouse_pos.x - pan_start_screen.x, .y = mouse_pos.y - pan_start_screen.y };
            move_plot(&cfg, (Vector2){
                .x = pan_start_pos.x - pan_delta.x / cfg.grid.size.x,
                .y = pan_start_pos.y + pan_delta.y / cfg.grid.size.y,
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

            size_t cap = (npolys > POLY_CAP) ? POLY_CAP : npolys;

            for (size_t i = 0; i < cap; i++) {
                float sound_y = (float)wst_poly_eval(&polys[i], sound_x);
                float sound_y_lin = (sound_y - cfg.grid.rect.start.y) / (cfg.grid.rect.end.y - cfg.grid.rect.start.y);
                /* Stop sound if plot is too far off screen */
                if (sound_y_lin > 2.f || sound_y_lin < -1.f)
                    synth->target_freq[i] = 0.f;
                else
                    synth->target_freq[i] = cfg.audio.low_freq * powf(cfg.audio.high_freq / cfg.audio.low_freq, sound_y_lin);
            }
        } else {
            for (size_t i = 0; i < POLY_CAP; i++) {
                synth->target_freq[i] = 0.f;
            }
        }

        BeginDrawing();
            ClearBackground(BLACK);

            if (cfg.grid.draw_background)
                draw_background_grid(&cfg);
            draw_main_axes(&cfg);
            draw_axes_number_lines(&cfg);
            draw_axes_labels(&cfg);

            for (size_t i = 0; i < npolys; i++) {
                draw_poly_plot(&cfg, &polys[i], poly_colors[i % ARR_LEN(poly_colors)]);
                draw_plot_roots(&cfg, &sols[i]);

                // Draw polynomial expression
                DrawText(TextFormat("%s", poly_pretty[i]),
                        10, 10 + (cfg.font.title.sz + 2) * (int)i, cfg.font.title.sz, LIGHTGRAY);

                draw_plot_deriv(&cfg, &polys[i], &derivs[i],
                                mouse_pos, TextFormat("y%zu'", i));
            }

            if (playing_sound) {
                Vector2 p = pos_to_screen(&cfg, (Vector2){ .x = sound_x, .y = 0.f });
                DrawLineEx((Vector2){ .x = p.x, .y = 0, },
                           (Vector2){ .x = p.x, .y = (float)cfg.win.h },
                           1.f, WHITE);
            }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
