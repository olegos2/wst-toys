#include "file_sort.h"
#include "toys/argparse.h"
#include "toys/file.h"
#include "toys/sort.h"
#include "toys/debug.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wchar.h>


/**
 * Find a suitable UTF-8 locale and set it. Prefers current/default.
 */
static bool setup_locale(void);

/**
 * Sort previously read and decoded `lines` and write them to
 * previously opened file `out`.
 * Does not close file or free lines.
 *
 * @return `true` if everything was written, `false` on failure.
 */
static bool write_sorted_output(FILE *out, WstLine *lines, size_t nlines,
                                const WstFile *content);


/**
 * - Read input file by path into a buffer,
 * - split it into strings,
 * - decode into wide char strings,
 * - sort strings in multiple ways,
 * - catenate original file contents,
 * - write all 3 variants to one output file.
 */
static int run_file_sort(const char *in_path, const char *out_path)
{
    assert(in_path != NULL);

    WstFile content = { 0 };
    if (!wst_file_open(in_path, &content))
        return EXIT_FAILURE;

    size_t nlines = 0;
    WstLine *lines = split_lines(content.data, content.size, &nlines);
    if (nlines > 0 && lines == NULL) {
        wst_file_close(&content);
        return EXIT_FAILURE;
    }

    if (!decode_lines(lines, nlines)) {
        free_lines(lines, nlines);
        wst_file_close(&content);
        return EXIT_FAILURE;
    }
    LOG_I("'%s': %zu bytes, %zu lines",
          in_path, content.size, nlines);

    FILE *out = (out_path != NULL) ? fopen(out_path, "w") : stdout;
    if (out == NULL) {
        LOG_E("cannot open '%s' for writing: %s", out_path, strerror(errno));
        free_lines(lines, nlines);
        wst_file_close(&content);
        return EXIT_FAILURE;
    }
    LOG_D("opened '%s' for writing", out_path);

    int ret = EXIT_SUCCESS;

    if (!write_sorted_output(out, lines, nlines, &content))
        ret = EXIT_FAILURE;

    if (out != stdout && fclose(out) != 0) {
        LOG_E("failed closing '%s': %s", out_path, strerror(errno));
    }

    free_lines(lines, nlines);
    wst_file_close(&content);

    if (ret == EXIT_SUCCESS && out_path != NULL)
        LOG_I("wrote '%s'", out_path);

    return ret;
}


int main(int argc, char **argv)
{
    bool help = false;
    bool verbose = false;
    const char *debug_filename = NULL;
    const char *debug_color = NULL;
    const char *in_path = NULL;
    /* NULL out_path maps to stdout */
    const char *out_path = NULL;

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
            .type = ARG_STRING,
            .dest = &debug_color,
            .short_name = "-c",
            .long_name = "--color",
            .description = "set debug messages coloring mode (never, always, default: auto)",
        },
        {
            .type = ARG_POSITIONAL,
            .dest = &in_path,
            .long_name = "in_filename",
            .description = "input file path to read",
            .required = true,
        },
        {
            .type = ARG_POSITIONAL,
            .dest = &out_path,
            .long_name = "out_filename",
            .description = "output file path to write",
        },
    };

    ArgParser parser = {
        .prog = argv[0],
        .opts = opts,
        .nopts = ARR_LEN(opts),
    };

    if (!argparse_parse(&parser, argc, argv)) {
        /* Check if help flag was parsed, even if other required args are missing. */
        if (help)
            argparse_print_help(&parser);
        else
            fprintf(stderr, "%s, run '%s --help' for usage\n", parser.error, argv[0]);
        return EXIT_FAILURE;
    }

    if (help) {
        argparse_print_help(&parser);
        return EXIT_SUCCESS;
    }

    if (debug_color != NULL) {
        if (strcmp(debug_color, "always") == 0)
            wst_log_enable_color(WST_LOG_COLOR_ON);
        else if (strcmp(debug_color, "never") == 0)
            wst_log_enable_color(WST_LOG_COLOR_OFF);
    }

    if (verbose)
        wst_log_set_max_prio(WST_LOG_VERBOSE);

    if (!wst_log_open(debug_filename))
        LOG_W("failed to open log file for writing");

    if (!setup_locale())
        return EXIT_FAILURE;

    return run_file_sort(in_path, out_path);
}

                
static bool write_sorted_output(FILE *out, WstLine *lines, size_t nlines,
                                const WstFile *content)
{
    static const char *separator = "/* ----------------------------- */\n";

    wst_qsort(lines, nlines, sizeof(*lines), cmp_letters_fwd);
    if (!write_alpha_lines(out, lines, nlines) ||
        fwrite(separator, 1, strlen(separator), out) != strlen(separator))
    {
        LOG_E("failed to write forward sorted lines");
        return false;
    }
    LOG_I("wrote forward sorted lines");

    wst_qsort(lines, nlines, sizeof(*lines), cmp_letters_rev);
    if (!write_alpha_lines(out, lines, nlines) ||
        fwrite(separator, 1, strlen(separator), out) != strlen(separator))
    {
        LOG_E("failed to write backward sorted lines");
        return false;
    }
    LOG_I("wrote backward sorted lines");

    if (content->size > 0 &&
        fwrite(content->data, 1, content->size, out) != content->size)
    {
        LOG_E("failed to write original input contents to output");
        return false;
    }
    LOG_I("catenated input to output");
    return true;
}


static bool setup_locale(void)
{
    static const char *try_locales[] = {
        "", "C.UTF-8", "C.utf8", "en_US.UTF-8", "en_US.utf8",
    };

    for (size_t i = 0; i < ARR_LEN(try_locales); i++) {
        if (setlocale(LC_CTYPE, try_locales[i]) == NULL)
            continue;

        LOG_D("ctype locale: '%s'", setlocale(LC_CTYPE, NULL));
        return true;
    }

    LOG_E("no UTF-8 locale available");
    return false;
}
