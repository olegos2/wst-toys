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


static void test_parse(void)
{
    ArgParser p;
    argparse_init(&p, "prog");

    bool verbose = false;
    int num = 0;
    const char *file = NULL;
    argparse_add(&p, &(ArgOption){
        .type = ARG_SWITCH, .dest = &verbose,
        .short_name = "-v", .long_name = "--verbose",
        .description = "be loud",
    });
    argparse_add(&p, &(ArgOption){
        .type = ARG_INT, .dest = &num,
        .short_name = "-n", .long_name = "--num",
        .description = "a number",
    });
    argparse_add(&p, &(ArgOption){
        .type = ARG_POSITIONAL, .dest = &file, .required = true,
        .long_name = "file",
        .description = "input file",
    });

    char *argv1[] = { "prog", "-v", "--num=42", "in.txt" };
    CHECK(argparse_parse(&p, 4, argv1), "attached forms parse");
    CHECK(verbose, "switch set");
    CHECK(num == 42, "int attached form");
    CHECK(file != NULL && strcmp(file, "in.txt") == 0, "positional filled");

    char *argv2[] = { "prog", "--num", "-7", "--", "-x" };
    CHECK(argparse_parse(&p, 5, argv2), "separate value and -- parse");
    CHECK(num == -7, "negative int");
    CHECK(strcmp(file, "-x") == 0, "-- ends option parsing");

    /* dests are reset between parses */
    char *argv3[] = { "prog", "in.txt" };
    CHECK(argparse_parse(&p, 2, argv3), "plain parse");
    CHECK(!verbose && num == 0, "dests reset");
}

static void test_errors(void)
{
    ArgParser p;
    argparse_init(&p, "prog");

    bool force = false;
    int num = 0;
    const char *file = NULL;
    argparse_add(&p, &(ArgOption){
        .type = ARG_SWITCH, .dest = &force,
        .short_name = "-f", .long_name = "--force",
        .description = "overwrite",
    });
    argparse_add(&p, &(ArgOption){
        .type = ARG_INT, .dest = &num,
        .short_name = "-n", .long_name = "--num",
        .description = "a number",
    });
    argparse_add(&p, &(ArgOption){
        .type = ARG_POSITIONAL, .dest = &file, .required = true,
        .long_name = "file",
        .description = "input file",
    });

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

int main(void)
{
    test_parse();
    test_errors();
    if (failures == 0) {
        printf("all tests passed\n");
        return 0;
    }
    printf("%d test(s) failed\n", failures);
    return 1;
}
