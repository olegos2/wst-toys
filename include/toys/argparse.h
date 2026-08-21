#ifndef TOYS_ARGPARSE_H
#define TOYS_ARGPARSE_H

#include <stdbool.h>


/** Max options, including positionals, one parser accepts. */
#define ARG_MAX_OPTIONS 16

/** Type of argument/option for parser. */
typedef enum {
    /** flag, dest is `bool *`, set true when present */
    ARG_SWITCH,
    /** option with a value, dest is `int *` */
    ARG_INT,
    /** operand without dashes, dest is `const char **` */
    ARG_POSITIONAL,
} ArgType;

typedef struct {
    ArgType type;
    /** Whether this option must be present in program args. */
    bool required;
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
 * Option can have extra value next to it with `=` or whitespace.
 * Named options can be finished with `--`, positional args are parsed without conversion.
 */
typedef struct {
    /** Name of the program that parser will use when showing help */
    const char *prog;
    ArgOption opts[ARG_MAX_OPTIONS];
    int nopts;
    /** Empty unless the last parse failed */
    char error[128];
} ArgParser;

/** Resets parser and records prog for usage output. */
void argparse_init(ArgParser *p, const char *prog);

/** Copies opt into the parser, aborts on overflow. */
void argparse_add(ArgParser *p, const ArgOption *opt);

/**
 * Parses argv[0..argc) against the registered options and fills their
 * dest variables. Value forms are -o 1, -o=1, --opt 1 and --opt=1,
 * a bare "--" ends option parsing, positionals fill in declaration
 * order. All dest variables are reset before parsing.
 *
 * Returns true on success, otherwise sets p->error and returns false.
 */
bool argparse_parse(ArgParser *p, int argc, char **argv);

/** Prints the usage line and the option table to stdout. */
void argparse_print_help(const ArgParser *p);

#endif /* TOYS_ARGPARSE_H */
