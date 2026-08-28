#include "toys/argparse.h"
#include "toys/debug.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPT_LABEL_LEN 128


/* Name to show in messages, prefers the long form. */
static const char *opt_name(const ArgOption *o)
{
    return o->long_name ? o->long_name : o->short_name;
}

/**
 * Find named option in parser
 *
 * @param [in] name Name of option to look for
 * @param [in] len Length of name (in case it's followed with `=`)
 */
static ArgOption *find_opt(ArgParser *p, const char *name, size_t len)
{
    for (int i = 0; i < p->nopts; i++) {
        const char *s = p->opts[i].short_name;
        const char *l = p->opts[i].long_name;
        if ((s && strlen(s) == len && strncmp(s, name, len) == 0) ||
            (l && strlen(l) == len && strncmp(l, name, len) == 0))
            return &p->opts[i];
    }
    return NULL;
}

/* Single-insertion error messages, arg may be NULL */
static void err_set(ArgParser *p, const char *fmt, ...)
{
    va_list args = { 0 };
    va_start(args, fmt);
    vsnprintf(p->error, sizeof(p->error), fmt, args);
    va_end(args);
}

/* strtol over the whole value, rejects trailing garbage and overflow */
static bool parse_int(ArgParser *p, const ArgOption *o, const char *val)
{
    char *end = NULL;
    errno = 0;
    long v = strtol(val, &end, 10);
    if (errno != 0 || end == val || *end != '\0' || v < INT_MIN || v > INT_MAX) {
        err_set(p, "value of %s must be an integer", opt_name(o));
        return false;
    }
    *(int *)o->dest = (int)v;
    return true;
}

/**
 * Parses option in beginning of argv and sets its dest field.
 *
 * @param [in] argc number of args starting from current option arg
 * @param [in] argv array of args where argv[0] is current option name
 * @param [out] opt set to found option by name if any, or kept unchanged
 * @return number of args consumed, where zero means error
 */
static int parse_named_option(ArgParser *p, int argc, char **argv, ArgOption **opt)
{
    assert(argv != NULL);

    /* handle `-o=v` syntax, or expect `-o v` otherwise. */
    const char *eq = strchr(argv[0], '=');
    size_t name_len = eq ? (size_t)(eq - argv[0]) : strlen(argv[0]);

    /* try to find option by name. */
    ArgOption *o = find_opt(p, argv[0], name_len);
    if (o == NULL) {
        err_set(p, "unknown option %s", argv[0]);
        return 0;
    }
    *opt = o;

    int ret = 1;

    const char *val = NULL;
    if (o->type == ARG_SWITCH) {
        *(bool *)o->dest = true;
        return ret;
    }

    /* Parse value following option name (with `=` or in next arg) */
    if (eq != NULL)
        val = eq + 1;
    else if (argc >= 2) {
        val = argv[1];
        ret = 2;
    } else {
        err_set(p, "missing value for %s", opt_name(o));
        return 0;
    }

    switch (o->type) {
    case ARG_INT:
        if (!parse_int(p, o, val))
            return 0;
        break;
    case ARG_STRING:
        *(const char **)o->dest = val;
        break;
    default:
        assert(0 && "Unreachable/bad option type");
    }

    return ret;
}

bool argparse_parse(ArgParser *p, int argc, char **argv)
{
    assert(p != NULL);

    p->error[0] = '\0';
    p->rest = NULL;
    p->nrest = 0;

    /* reset dests to zeros so repeated parses start clean */
    for (int i = 0; i < p->nopts; i++) {
        ArgOption *o = &p->opts[i];
        o->seen = false;
        if (o->type == ARG_SWITCH)
            *(bool *)o->dest = false;
        else if (o->type == ARG_INT)
            *(int *)o->dest = 0;
        else
            *(const char **)o->dest = NULL;
    }

    /* whether named options were all parsed */
    bool opts_done = false;

    /* with capture_rest the tail after the last positional stays raw,
     * so subcommand args like -6 never read as options */
    int npos_total = 0;
    int npos_filled = 0;
    for (int j = 0; j < p->nopts; j++)
        if (p->opts[j].type == ARG_POSITIONAL)
            npos_total++;

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];

        bool raw = opts_done ||
                   (p->capture_rest && npos_total > 0 &&
                    npos_filled == npos_total);

        if (!raw && strcmp(arg, "--") == 0) {
            opts_done = true;
            continue;
        }

        if (!raw && arg[0] == '-' && arg[1] != '\0') {
            ArgOption *opt;
            int ret = parse_named_option(p, argc - i, argv + i, &opt);
            if (!ret) return false;
            /* Skip one extra arg for option of format `-o v`. */
            i += ret - 1;
            opt->seen = true;
            continue;
        }

        /* first unfilled positional in declaration order */
        ArgOption *o = NULL;
        for (int j = 0; j < p->nopts; j++) {
            if (p->opts[j].type == ARG_POSITIONAL && !p->opts[j].seen) {
                o = &p->opts[j];
                break;
            }
        }
        if (o == NULL) {
            if (p->capture_rest) {
                /* subcommand tail, hand it over uninterpreted */
                p->rest = &argv[i];
                p->nrest = argc - i;
                break;
            }
            err_set(p, "unexpected argument %s", arg);
            return false;
        }
        LOG_D("Got positional arg %s", arg);
        *(const char **)o->dest = arg;
        o->seen = true;
        npos_filled++;
    }

    for (int i = 0; i < p->nopts; i++) {
        if (p->opts[i].required && !p->opts[i].seen) {
            err_set(p, "missing required %s", opt_name(&p->opts[i]));
            return false;
        }
    }
    return true;
}

static size_t label_len(const ArgOption *o)
{
    if (o->type == ARG_POSITIONAL)
        return strlen(o->long_name) + 2;
    else if (o->short_name && o->long_name)
        return strlen(o->short_name) + strlen(o->long_name) + 2;
    else
        return strlen(opt_name(o));
}

/* Option column label, positionals show as <name> */
static void format_label(const ArgOption *o, char *buf, size_t size)
{
    if (o->type == ARG_POSITIONAL)
        snprintf(buf, size, "<%s>", o->long_name);
    else if (o->short_name && o->long_name)
        snprintf(buf, size, "%s, %s", o->short_name, o->long_name);
    else
        snprintf(buf, size, "%s", opt_name(o));
}

void argparse_print_help(const ArgParser *p)
{
    assert(p != NULL);

    printf("Usage: %s [options]", p->prog);
    for (int i = 0; i < p->nopts; i++)
        if (p->opts[i].type == ARG_POSITIONAL)
            printf(" %s", p->opts[i].long_name);
    printf("\n\nOptions:\n");

    char **labels;
    labels = calloc((size_t)p->nopts, sizeof(char *));
    if (labels == NULL) {
        LOG_E("calloc: %s", strerror(errno));
        printf("Failed to alloc option labels\n");
        return;
    }
    size_t width = 0;
    for (int i = 0; i < p->nopts; i++) {
        size_t len = label_len(&p->opts[i]);
        labels[i] = calloc(len + 1, sizeof(char));
        format_label(&p->opts[i], labels[i], len + 1);
        if (len > width)
            width = len;
    }

    /* TODO: Add description wrapping. */
    for (int i = 0; i < p->nopts; i++)
        printf("  %-*s  %s\n", (int)width, labels[i], p->opts[i].description);

    for (int i = 0; i < p->nopts; i++)
        free(labels[i]);
    free(labels);
}
