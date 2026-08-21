#include "toys/argparse.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void argparse_init(ArgParser *p, const char *prog)
{
    memset(p, 0, sizeof(*p));
    p->prog = prog;
}

void argparse_add(ArgParser *p, const ArgOption *opt)
{
    assert(p->nopts < ARG_MAX_OPTIONS);
    p->opts[p->nopts++] = *opt;
}


/* Name to show in messages, prefers the long form. */
static const char *opt_name(const ArgOption *o)
{
    return o->long_name ? o->long_name : o->short_name;
}

/**
 * Find named option in parser
 *
 * @param [in] name Name of option to look for
 * @param [in] len Length of name buffer
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
static void err_set(ArgParser *p, const char *fmt, const char *arg)
{
    snprintf(p->error, sizeof(p->error), fmt, arg ? arg : "");
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

bool argparse_parse(ArgParser *p, int argc, char **argv)
{
    p->error[0] = '\0';
    p->rest = NULL;
    p->nrest = 0;

    bool seen[ARG_MAX_OPTIONS] = { false };

    /* reset dests to zeros so repeated parses start clean */
    for (int i = 0; i < p->nopts; i++) {
        ArgOption *o = &p->opts[i];
        if (o->type == ARG_SWITCH)
            *(bool *)o->dest = false;
        else if (o->type == ARG_INT)
            *(int *)o->dest = 0;
        else
            *(const char **)o->dest = NULL;
    }

    /* whether named options were all parsed */
    bool opts_done = false;

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];

        /* check for `--` that ends named options */
        if (!opts_done && strcmp(arg, "--") == 0) {
            opts_done = true;
            continue;
        }

        if (!opts_done && arg[0] == '-' && arg[1] != '\0') {
            /* handle `-o=v` syntax, or expect `-o v` otherwise. */
            const char *eq = strchr(arg, '=');
            size_t name_len = eq ? (size_t)(eq - arg) : strlen(arg);

            /* try to find option by name. */
            ArgOption *o = find_opt(p, arg, name_len);
            if (o == NULL) {
                err_set(p, "unknown option %s", arg);
                return false;
            }

            /* handle currently available named option types and parse into dest. */
            if (o->type == ARG_SWITCH) {
                *(bool *)o->dest = true;
            } else {
                const char *val = NULL;
                if (eq != NULL) {
                    val = eq + 1;
                } else if (i + 1 < argc) {
                    val = argv[++i];
                } else {
                    err_set(p, "missing value for %s", opt_name(o));
                    return false;
                }
                if (!parse_int(p, o, val))
                    return false;
            }
            seen[o - p->opts] = true;
            continue;
        }

        /* first unfilled positional in declaration order */
        ArgOption *o = NULL;
        for (int j = 0; j < p->nopts; j++) {
            if (p->opts[j].type == ARG_POSITIONAL && !seen[j]) {
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
        *(const char **)o->dest = arg;
        seen[o - p->opts] = true;
    }

    for (int i = 0; i < p->nopts; i++) {
        if (p->opts[i].required && !seen[i]) {
            err_set(p, "missing required %s", opt_name(&p->opts[i]));
            return false;
        }
    }
    return true;
}


/* Option column label, positionals show as <name> */
static void format_label(const ArgOption *o, char *buf, size_t size)
{
    if (o->type == ARG_POSITIONAL) {
        snprintf(buf, size, "<%s>", o->long_name);
    } else if (o->short_name && o->long_name) {
        snprintf(buf, size, "%s, %s", o->short_name, o->long_name);
    } else {
        snprintf(buf, size, "%s", opt_name(o));
    }
}

void argparse_print_help(const ArgParser *p)
{
    printf("Usage: %s [options]", p->prog);
    for (int i = 0; i < p->nopts; i++)
        if (p->opts[i].type == ARG_POSITIONAL)
            printf(" %s", p->opts[i].long_name);
    printf("\n\nOptions:\n");

    char labels[ARG_MAX_OPTIONS][32];
    int width = 0;
    for (int i = 0; i < p->nopts; i++) {
        format_label(&p->opts[i], labels[i], sizeof(labels[i]));
        int len = (int)strlen(labels[i]);
        if (len > width)
            width = len;
    }

    for (int i = 0; i < p->nopts; i++)
        printf("  %-*s  %s\n", width, labels[i], p->opts[i].description);
}
