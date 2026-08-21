#include "toys/debug.h"
#include "toys/solve.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


/* Single pass expression parser: the lexer streams tokens into recursive descent and
 * every production returns its ToysPoly directly. */

/* Nesting cap for '(' and '-' chains, which recurse per character. */
#define TOYS_EXPR_MAX_DEPTH 256

/** Types of tokens that lexer can parse. */
typedef enum {
    TOYS_EXPR_NUM,
    TOYS_EXPR_VAR,
    TOYS_EXPR_OP,
    TOYS_EXPR_LPAREN,
    TOYS_EXPR_RPAREN,
    TOYS_EXPR_EQ,
    TOYS_EXPR_END,
} TokenType;

typedef struct {
    TokenType type;
    size_t pos;
    union {
        double val;
        char op;
    };
} Token;

/** State of the expr parser. */
typedef struct {
    /* Input expression string for parser */
    const char *s;
    /* Length of s */
    size_t len;
    /* Position where parser is stopped now. */
    size_t pos;
    /* Depth of expr nesting at current pos. */
    size_t depth;
    /* Next token lookahead. */
    Token lookahead;
    /* Whether lookahead is already filled. */
    bool have;
    /* [out] position in string at which first error happened. */
    size_t *err_pos;
    /* [out] error string. */
    const char *err_msg;
} Parser;

WstPoly wst_poly_scale(const WstPoly *a, double s)
{
    WstPoly r = *a;
    for (int i = 0; i <= WST_SOLVE_MAX_DEGREE; i++)
        r.coeffs[i] *= s;
    return r;
}

WstPoly wst_poly_add(const WstPoly *a, const WstPoly *b)
{
    WstPoly r = *a;
    int degree = (a->degree > b->degree) ? a->degree : b->degree;
    for (int i = 0; i <= degree; i++)
        r.coeffs[i] = a->coeffs[i] + b->coeffs[i];
    r.degree = degree;
    return r;
}

WstPoly wst_poly_sub(const WstPoly *a, const WstPoly *b)
{
    WstPoly r = *a;
    int degree = (a->degree > b->degree) ? a->degree : b->degree;
    for (int i = 0; i <= degree; i++)
        r.coeffs[i] = a->coeffs[i] - b->coeffs[i];
    r.degree = degree;
    return r;
}

bool wst_poly_mul(const WstPoly *a, const WstPoly *b, WstPoly *out)
{
    int degree = a->degree + b->degree;
    if (degree > WST_SOLVE_MAX_DEGREE)
        return false;

    memset(out, 0, sizeof(*out));
    out->degree = degree;
    for (int i = 0; i <= a->degree; i++)
        for (int j = 0; j <= b->degree; j++)
            out->coeffs[i + j] += a->coeffs[i] * b->coeffs[j];
    return true;
}

static void err_set(Parser *p, size_t pos, const char *msg)
{
    /* First error sticks. */
    if (p->err_msg == NULL) {
        p->err_msg = msg;
        *p->err_pos = pos;
    }
}

/**
 * Reads a number token with strtod, then checks the consumed span
 * only holds digits, dot and e/E. Sets parser error on failure.
 * @param [inout] p parser state
 * @param [out] tok stores resulting number token on success
 */
static bool lex_number(Parser *p, Token *tok)
{
    char *end = NULL;
    double val = strtod(p->s + p->pos, &end);
    size_t span = (size_t)(end - (p->s + p->pos));

    if (span == 0)
        return false;

    for (size_t i = 0; i < span; i++) {
        char c = p->s[p->pos + i];
        if (!((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E')) {
            err_set(p, p->pos, "unexpected character");
            return false;
        }
    }

    tok->type = TOYS_EXPR_NUM;
    tok->pos = p->pos;
    tok->val = val;
    p->pos += span;
    return true;
}

/* Fills *tok with the next token, returns false on an unexpected character. */
static bool lex_next(Parser *p, Token *tok)
{
    /* Skip whitespace/newlines */
    while (p->pos < p->len) {
        char c = p->s[p->pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            p->pos++;
        else
            break;
    }

    tok->pos = p->pos;
    if (p->pos >= p->len) {
        tok->type = TOYS_EXPR_END;
        return true;
    }

    char c = p->s[p->pos];
    /* Branch into number parsing early */
    if ((c >= '0' && c <= '9') || c == '.')
        return lex_number(p, tok);

    switch (c) {
    case 'x':
        tok->type = TOYS_EXPR_VAR;
        break;
    case '+': case '-': case '*': case '/': case '^':
        tok->type = TOYS_EXPR_OP;
        tok->op = c;
        break;
    case '(':
        tok->type = TOYS_EXPR_LPAREN;
        break;
    case ')':
        tok->type = TOYS_EXPR_RPAREN;
        break;
    case '=':
        tok->type = TOYS_EXPR_EQ;
        break;
    default:
        err_set(p, p->pos, "unexpected character");
        return false;
    }
    p->pos++;
    return true;
}

static void lex_peek(Parser *p, Token *tok)
{
    if (!p->have) {
        if (!lex_next(p, &p->lookahead)) {
            /* lex errors surface as END, err_msg is re-checked at the end */
            p->lookahead.type = TOYS_EXPR_END;
            p->lookahead.pos = p->pos;
        }
        p->have = true;
    }
    *tok = p->lookahead;
}

static Token lex_take(Parser *p)
{
    Token tok;
    lex_peek(p, &tok);
    p->have = false;
    return tok;
}

const char *wst_expr_to_poly(const char *s, size_t len, WstPoly *out, size_t *err_pos)
{
    LOG_E("No");
    return NULL;
}
