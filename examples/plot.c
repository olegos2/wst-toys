#include "toys/poly.h"

#include <raylib.h>
#include <raymath.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_BUFFER_SIZE 4096
#define POLY_BUF_LEN 256

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
    float low_freq;
    float high_freq;
} PlotAudioConfig;

typedef struct {
    PlotWindowConfig win;
    PlotGridConfig grid;
    PlotFontConfig font;
    PlotAudioConfig audio;
    /** Position of plot origin on screen. */
    float center_x;
    float center_y;
    float x_step;
} PlotConfig;

typedef struct {
    float freq;
    float target_freq;
    float phase;
    float interp_factor;
} AudioSynth;

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

    text = "x";
    text_width = MeasureText(text, cfg->font.title);
    DrawText(text, (int)(cfg->center_x + cfg->grid.end_x * (float)cfg->grid.size) - text_width - 4,
             (int)cfg->center_y + 2, cfg->font.title, GREEN);

    text = "y";
    text_width = MeasureText(text, cfg->font.title);
    DrawText(text, (int)cfg->center_x - text_width - 2, 4, cfg->font.title, GREEN);
}

static void draw_poly_plot(const PlotConfig *cfg, const WstPoly *poly)
{
    assert(cfg != NULL);
    assert(poly != NULL);

    float saved_i = cfg->grid.start_x;
    float saved_j = (float)wst_poly_eval(poly, cfg->grid.start_x);

    for (float i = (float)cfg->grid.start_x + cfg->x_step; i <= cfg->grid.end_x; i += cfg->x_step) {
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

static AudioSynth audio_synth = {
    .interp_factor = 0.01f,
}, *synth = &audio_synth;

static void plot_audio_callback(void *frames_out, unsigned int frame_count)
{
    float *buf = (float *)frames_out;
    
    for (unsigned int i = 0; i < frame_count; i++) {
        synth->freq += (synth->target_freq - synth->freq) * synth->interp_factor;
        buf[i] = sinf(synth->phase);
        synth->phase += (2.f * PI * synth->freq) / AUDIO_SAMPLE_RATE;

        if (synth->phase > 2.f * PI)
            synth->phase -= 2.f * PI;
    }
}

int solve_run_plot(const WstPoly *poly, const WstSolution *sol)
{
    char poly_pretty[POLY_BUF_LEN];
    wst_poly_print(poly_pretty, sizeof(poly_pretty), "y", poly, true);
    printf("%s\n", poly_pretty);

    SetTargetFPS(60);

    PlotConfig plot_cfg = {
        .win = {
            .w = 800,
            .h = 600,
        },
        .grid = {
            .size = 40,
        },
        .audio = {
            .low_freq = 150.f,
            .high_freq = 5000.f,
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

    plot_cfg.grid.start_x = -len_x / 2;
    plot_cfg.grid.end_x = plot_cfg.grid.start_x + len_x;
    plot_cfg.grid.start_y = -len_y / 2;
    plot_cfg.grid.end_y = plot_cfg.grid.start_y + len_y;

    InitWindow(plot_cfg.win.w, plot_cfg.win.h, "Polynomial plot");

    InitAudioDevice();

    SetAudioStreamBufferSizeDefault(AUDIO_BUFFER_SIZE);
    AudioStream stream = LoadAudioStream(AUDIO_SAMPLE_RATE, 32, 1);

    SetAudioStreamCallback(stream, plot_audio_callback);
    PlayAudioStream(stream);

    float sound_x = plot_cfg.grid.start_x;

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(BLACK);

            draw_background_grid(&plot_cfg);
            draw_main_axes(&plot_cfg);
            draw_axes_number_lines(&plot_cfg);
            draw_axes_labels(&plot_cfg);
            draw_poly_plot(&plot_cfg, poly);
            draw_plot_roots(&plot_cfg, sol);

            // Draw polynomial expression
            DrawText(TextFormat("%s", poly_pretty),
                     10, 10, plot_cfg.font.title, YELLOW);

            // sound_x += 0.01f;
            // float sound_y = (float)wst_poly_eval(&poly, sound_x);
            // float sound_y_lin = sound_y - 
            // synth->target_freq = 
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
