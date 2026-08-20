#ifndef TOYS_EXPR_H
#define TOYS_EXPR_H

#include "toys/solve.h"

#include <stddef.h>


/**
 * Parses s[0..len) as an expression over x using + - * / ( ) = ^ and
 * reduces it to a polynomial; "a = b" is solved as a - b = 0.
 *
 * Returns 1 on success and fills in *out. On failure returns 0 and
 * stores the byte offset of the problem in *err_pos and a static
 * message in *err_msg.
 */
int toys_expr_to_poly(const char *s, size_t len, ToysPoly *out,
                      size_t *err_pos, const char **err_msg);

#endif /* TOYS_EXPR_H */