#include "tests_common.h"

#include "toys/argparse.h"

#include <stdio.h>
#include <string.h>


static bool try_parse(ArgParser *p, int argc, char *argv[], const char *msg, const char *file, int line)
{
    if (!argparse_parse(p, argc, argv)) {
        fprintf(stderr, "FAIL %s: %s (%s:%d)\n", msg, p->error, file, line);
        failures++;
        return false;
    }
    return true;
}

#define CHECK_PARSE(p, argc, argv, msg) \
    try_parse(p, argc, argv, msg, __FILE__, __LINE__)


static void test_parse(void)
{
    bool verbose = false;
    int num = 0;
    const char *file = NULL;
    const char *mode = NULL;

    ArgOption opts[] = {
        {
            .type = ARG_SWITCH,
            .dest = &verbose,
            .short_name = "-v",
            .long_name = "--verbose",
            .description = "be loud",
        },
        {
            .type = ARG_INT,
            .dest = &num,
            .short_name = "-n",
            .long_name = "--num",
            .description = "a number",
        },
        {
            .type = ARG_STRING,
            .dest = &mode,
            .short_name = "-m",
            .long_name = "--mode",
            .description = "some mode",
        },
        {
            .type = ARG_POSITIONAL,
            .dest = &file,
            .required = true,
            .long_name = "file",
            .description = "input file",
        },
    };

    ArgParser p = {
        .prog = "prog",
        .opts = opts,
        .nopts = ARR_LEN(opts),
    };

    char *argv1[] = { "prog", "-v", "--num=42", "--mode=test", "in.txt" };
    if (CHECK_PARSE(&p, ARR_LEN(argv1), argv1, "attached forms parse")) {
        CHECK(verbose, "switch set");
        CHECK(num == 42, "int attached form");
        CHECK(strncmp(mode, "test", 4) == 0, "string attached form");
        CHECK(file != NULL && strcmp(file, "in.txt") == 0, "positional filled");
    }

    char *argv2[] = { "prog", "--num", "-7", "--mode", "", "--", "-x" };
    if (CHECK_PARSE(&p, ARR_LEN(argv2), argv2, "separate value and -- parse")) {
        CHECK(num == -7, "negative int");
        CHECK(*mode == '\0', "empty string");
        CHECK(file != NULL && strcmp(file, "-x") == 0, "-- ends option parsing");
    }

    /* dests are reset between parses */
    char *argv3[] = { "prog", "in.txt" };
    if (CHECK_PARSE(&p, ARR_LEN(argv3), argv3, "plain parse")) {
        CHECK(!verbose && num == 0 && mode == NULL, "dests reset");
    }
}

static void test_errors(void)
{
    bool force = false;
    int num = 0;
    const char *file = NULL;

    ArgOption opts[] = {
        {
            .type = ARG_SWITCH,
            .dest = &force,
            .short_name = "-f",
            .long_name = "--force",
            .description = "overwrite",
        },
        {
            .type = ARG_INT,
            .dest = &num,
            .short_name = "-n",
            .long_name = "--num",
            .description = "a number",
        },
        {
            .type = ARG_POSITIONAL,
            .dest = &file,
            .required = true,
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
    CHECK(!argparse_parse(&p, ARR_LEN(argv1), argv1), "unknown option fails");
    CHECK(strcmp(p.error, "unknown option --nope") == 0, "unknown option message");

    char *argv2[] = { "prog" };
    CHECK(!argparse_parse(&p, ARR_LEN(argv2), argv2), "missing required fails");
    CHECK(strcmp(p.error, "missing required file") == 0, "required message");

    char *argv3[] = { "prog", "-n" };
    CHECK(!argparse_parse(&p, ARR_LEN(argv3), argv3), "missing value fails");

    char *argv4[] = { "prog", "-n=abc", "f" };
    CHECK(!argparse_parse(&p, ARR_LEN(argv4), argv4), "bad int fails");

    char *argv5[] = { "prog", "a", "b" };
    CHECK(!argparse_parse(&p, ARR_LEN(argv5), argv5), "extra positional fails");
}

static void test_rest(void)
{
    bool help = false;
    ArgOption opts[] = {
        {
            .type = ARG_SWITCH,
            .dest = &help,
            .short_name = "-h",
            .long_name = "--help",
            .description = "help",
        },
    };

    ArgParser p = {
        .prog = "prog",
        .opts = opts,
        .nopts = ARR_LEN(opts),
    };

    char *argv1[] = { "prog", "a", "b" };
    CHECK(!argparse_parse(&p, ARR_LEN(argv1), argv1), "no capture by default");

    p.capture_rest = true;
    char *argv2[] = { "prog", "-h", "cmd", "-x", "y" };
    if (CHECK_PARSE(&p, ARR_LEN(argv2), argv2, "rest capture")) {
        CHECK(help, "switch before command");
        CHECK(p.nrest == 3, "rest count");
        CHECK(strcmp(p.rest[0], "cmd") == 0, "rest head");
        CHECK(strcmp(p.rest[1], "-x") == 0, "raw dash token in rest");
    }
}

static void test_subcommand_tail(void)
{
    /* with a declared positional, dash tokens after it stay raw */
    const char *cmd = NULL;
    ArgOption opts[] = {
        {
            .type = ARG_POSITIONAL,
            .dest = &cmd,
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

    char *argv[] = { "prog", "coeffs", "-6", "5" };

    if (CHECK_PARSE(&p, ARR_LEN(argv), argv, "dash tokens go raw after positional")) {
        CHECK(cmd != NULL && strcmp(cmd, "coeffs") == 0, "command captured");
        CHECK(p.nrest == 2, "raw tail length");
        CHECK(strcmp(p.rest[0], "-6") == 0, "negative number stays raw");
    }
}

int main(void)
{
    test_parse();
    test_errors();
    test_rest();
    test_subcommand_tail();
    return tests_summary();
}
