#ifndef CALC_PLOT_H
#define CALC_PLOT_H

#include "toys/poly.h"

#include <raylib.h>
#include <raymath.h>
#include <stddef.h>


/** How many polynomials can be plotted at once. */
#define POLY_CAP 8

#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_BUFFER_SIZE 4096


/** Rectangle defined using 2 corners. */
typedef struct {
    Vector2 start;
    Vector2 end;
} PlotRect;

/** Configuration of plot grid and viewport. */
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
    /** Whether to draw background grid or not. */
    bool draw_background;
} PlotGridConfig;

/** Plot window state. */
typedef struct {
    int w;
    int h;
} PlotWindowConfig;

/** Font template that also includes size. */
typedef struct {
    Font face;
    int sz;
} PlotFont;

/** Fonts used in plot. */
typedef struct {
    PlotFont title;
    PlotFont label;
} PlotFontConfig;

/** Audio configuration for plot. */
typedef struct {
    float low_freq;
    float high_freq;
} PlotAudioConfig;

/** Configuration for plot. */
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

/** Audio synth that can play up to `POLY_CAP` sine waves at same time. */
typedef struct {
    float freq[POLY_CAP];
    float target_freq[POLY_CAP];
    float phase[POLY_CAP];
    float interp_factor;
    float volume;
} AudioSynth;


/**
 * Runs graphical calculator that draws passed polynomials and their solutions.
 * 
 * @param[in] npolys Number of polynomials and solutions passed.
 * @param[in] polys Polynomials array.
 * @param[in] sols Solutions array with `npolys` elements.
 */
int solve_run_plot(size_t npolys, const WstPoly *polys, const WstSolution *sols);


#endif /* CALC_PLOT_H */
