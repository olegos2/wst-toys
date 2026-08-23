#ifndef TOYS_ARGPARSE_H
#define TOYS_ARGPARSE_H

#include <stdbool.h>

#define PARSER_ERR_LEN 128

/** Type of argument/option for parser. */
typedef enum {
    /** Flag, dest is `bool *`, set true when present */
    ARG_SWITCH,
    /** Option with a value, dest is `int *` */
    ARG_INT,
    /** Option with string value, dest is `const char **` */
    ARG_STRING,
    /** Operand without dashes, dest is `const char **` */
    ARG_POSITIONAL,
} ArgType;

typedef struct {
    ArgType type;
    /** Whether this option must be present in program args. */
    bool required;
    /** Runtime field for tracking whether flag appeared in argv */
    bool seen;
    /** Variable filled by parse, its type follows ArgType */
    void *dest;
    /** Token forms including dashes, either may be NULL */
    const char *short_name;
    const char *long_name;
    /** Description of option for user. */
    const char *description;
} ArgOption;

/**
 * Parser of command line arguments,
 * Uses single dash for short option names, double dash for long names,
 * Valid syntax for named options is -o 1, -o=1, --opt 1 and --opt=1,
 * Named options can be finished with `--`, positional args are parsed without conversion.
 * Dest variables are filled with zeros before parsing.
 * `capture_rest` can be set to parse all unexpected args into `rest` array.
 */
typedef struct {
    /** Name of the program that parser will use when showing help */
    const char *prog;
    // /** A string describing how rest of args will be parsed (if enabled) */
    // const char *rest_template;
    ArgOption *opts;
    int nopts;
    /** Empty unless the last parse failed */
    char error[PARSER_ERR_LEN];
    /** Gather leftover args into rest instead of failing */
    bool capture_rest;
    /** Args from the first leftover on, filled when capture_rest is set */
    char **rest;
    /** Length of rest args array. */
    int nrest;
} ArgParser;

/**
 * Parses argv and fills their dest variables.
 * Ints and bools are parsed, string pointers are set (strings not dup'ed).
 * Returns true on success, otherwise sets p->error and returns false.
 */
bool argparse_parse(ArgParser *p, int argc, char **argv);

/** Prints the usage line and the option table to stdout. */
void argparse_print_help(const ArgParser *p);

#endif /* TOYS_ARGPARSE_H */
