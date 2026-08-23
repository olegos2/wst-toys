#include "toys/argparse.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            failures++; \
        } \
    } while (0)

#define ARR_LEN(arr) (sizeof(arr) / sizeof(*arr))


static void test_parse(void)
{
    bool verbose = false;
    int num = 0;
    const char *file = NULL;
    const char *mode = NULL;

    ArgOption opts[] = {
        {
            .type = ARG_SWITCH, .dest = &verbose,
            .short_name = "-v", .long_name = "--verbose",
            .description = "be loud",
        },
        {
            .type = ARG_INT, .dest = &num,
            .short_name = "-n", .long_name = "--num",
            .description = "a number",
        },
        {
            .type = ARG_STRING, .dest = &mode,
            .short_name = "-m", .long_name = "--mode",
            .description = "some mode",
        },
        {
            .type = ARG_POSITIONAL, .dest = &file, .required = true,
            .long_name = "file",
            .description = "input file",
        },
    };

    ArgParser p = {
        .prog = "prog",
        .opts = opts,
        .nopts = ARR_LEN(opts),
    };

    // TODO: use file
    char *argv1[] = { "prog", "-v", "--num=42", "--mode=test", "in.txt" };
    CHECK(argparse_parse(&p, sizeof(argv1) / sizeof(*argv1), argv1), "attached forms parse");
    CHECK(verbose, "switch set");
    CHECK(num == 42, "int attached form");
    CHECK(strncmp(mode, "test", 4) == 0, "string attached form");
    CHECK(file != NULL && strcmp(file, "in.txt") == 0, "positional filled");

    char *argv2[] = { "prog", "--num", "-7", "--mode", "", "--", "-x" };
    CHECK(argparse_parse(&p, sizeof(argv2) / sizeof(*argv2), argv2), "separate value and -- parse");
    CHECK(num == -7, "negative int");
    CHECK(*mode == '\0', "empty string");
    CHECK(strcmp(file, "-x") == 0, "-- ends option parsing");

    /* dests are reset between parses */
    char *argv3[] = { "prog", "in.txt" };
    CHECK(argparse_parse(&p, sizeof(argv3) / sizeof(*argv3), argv3), "plain parse");
    CHECK(!verbose && num == 0 && mode == NULL, "dests reset");
}

static void test_errors(void)
{
    bool force = false;
    int num = 0;
    const char *file = NULL;

    ArgOption opts[] = {
        {
            .type = ARG_SWITCH, .dest = &force,
            .short_name = "-f", .long_name = "--force",
            .description = "overwrite",
        },
        {
            .type = ARG_INT, .dest = &num,
            .short_name = "-n", .long_name = "--num",
            .description = "a number",
        },
        {
            .type = ARG_POSITIONAL, .dest = &file, .required = true,
            .long_name = "file",
            .description = "input file",
        },
    };

    ArgParser p = {
        .prog = "prog",
        .opts = opts,
        .nopts = ARR_LEN(opts),
    };

    char *argv1[] = { "prog", "--nope" };
    CHECK(!argparse_parse(&p, 2, argv1), "unknown option fails");
    CHECK(strcmp(p.error, "unknown option --nope") == 0, "unknown option message");

    char *argv2[] = { "prog" };
    CHECK(!argparse_parse(&p, 1, argv2), "missing required fails");
    CHECK(strcmp(p.error, "missing required file") == 0, "required message");

    char *argv3[] = { "prog", "-n" };
    CHECK(!argparse_parse(&p, 2, argv3), "missing value fails");

    char *argv4[] = { "prog", "-n=abc", "f" };
    CHECK(!argparse_parse(&p, 3, argv4), "bad int fails");

    char *argv5[] = { "prog", "a", "b" };
    CHECK(!argparse_parse(&p, 3, argv5), "extra positional fails");
}

static void test_rest(void)
{
    bool help = false;
    ArgOption opts[] = {
        {
            .type = ARG_SWITCH, .dest = &help,
            .short_name = "-h", .long_name = "--help",
            .description = "help",
        },
    };

    ArgParser p = {
        .prog = "prog",
        .opts = opts,
        .nopts = ARR_LEN(opts),
    };

    /* capture off by default, extras still fail */
    char *argv1[] = { "prog", "a", "b" };
    CHECK(!argparse_parse(&p, sizeof(argv1) / sizeof(*argv1), argv1), "no capture by default");

    p.capture_rest = true;
    char *argv2[] = { "prog", "-h", "cmd", "-x", "y" };
    CHECK(argparse_parse(&p, sizeof(argv2) / sizeof(*argv2), argv2), "rest capture");
    CHECK(help, "switch before command");
    CHECK(p.nrest == 3, "rest count");
    CHECK(strcmp(p.rest[0], "cmd") == 0, "rest head");
    CHECK(strcmp(p.rest[1], "-x") == 0, "raw dash token in rest");
}

static void test_subcommand_tail(void)
{
    /* with a declared positional, dash tokens after it stay raw */
    const char *cmd = NULL;
    ArgOption opts[] = {
        {
            .type = ARG_POSITIONAL, .dest = &cmd,
            .long_name = "command",
            .description = "sub",
        },
    };

    ArgParser p = {
        .prog = "prog",
        .opts = opts,
        .nopts = ARR_LEN(opts),
        .capture_rest = true,
    };

    char *argv1[] = { "prog", "coeffs", "-6", "5" };
    CHECK(argparse_parse(&p, sizeof(argv1) / sizeof(*argv1), argv1), "dash tokens go raw after positional");
    CHECK(cmd != NULL && strcmp(cmd, "coeffs") == 0, "command captured");
    CHECK(p.nrest == 2, "raw tail length");
    CHECK(strcmp(p.rest[0], "-6") == 0, "negative number stays raw");
}

int main(void)
{
    test_parse();
    test_errors();
    test_rest();
    test_subcommand_tail();
    if (failures == 0) {
        printf("all tests passed\n");
        return 0;
    }
    printf("%d test(s) failed\n", failures);
    return 1;
}
