#include "toys/quad.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void print_solution(double a, double b, double c)
{
    double x1, x2;
    switch (toys_quad_solve(a, b, c, &x1, &x2)) {
    case TOYS_QUAD_TWO_ROOTS:
        printf("x1 = %g, x2 = %g\n", x1, x2);
        break;
    case TOYS_QUAD_ONE_ROOT:
        printf("x = %g\n", x1);
        break;
    case TOYS_QUAD_DEGENERATE:
        printf("x = %g (linear)\n", x1);
        break;
    case TOYS_QUAD_NO_ROOTS:
        printf("no real roots\n");
        break;
    case TOYS_QUAD_INVALID:
        printf("c=0: any x is a root, c!=0: no roots\n");
        break;
    }
}

static bool parse_coeff(const char *str, double *out)
{
    char *end = NULL;
    double val = strtod(str, &end);
    if (end == str || *end != '\0')
        return false;
    *out = val;
    return true;
}

static void run_interactive(void)
{
    char line[128];
    while (fgets(line, sizeof(line), stdin)) {
        double a, b, c;
        if (sscanf(line, "%lf %lf %lf", &a, &b, &c) != 3) {
            fprintf(stderr, "Expected three numbers: %s", line);
            continue;
        }
        print_solution(a, b, c);
    }
}

int main(int argc, char *argv[])
{
    if (argc == 4) {
        double a, b, c;
        if (!parse_coeff(argv[1], &a) ||
            !parse_coeff(argv[2], &b) ||
            !parse_coeff(argv[3], &c))
        {
            fprintf(stderr, "Invalid coefficient\n");
            return 1;
        }
        print_solution(a, b, c);
        return 0;
    }

    if (argc == 1) {
        run_interactive();
        return 0;
    }

    fprintf(stderr, "Usage: %s <a> <b> <c>\n", argv[0]);
    fprintf(stderr, "       %s          (solves \"a b c\" lines from stdin)\n", argv[0]);
    return 1;
}