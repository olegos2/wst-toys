#include "toys/debug.h"
#include "toys/solve.h"
#include "poly_impl.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>


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


/* Parser: recursive descent over the token stream, every production
 * returns the reduced polynomial directly.
 *
 *   expr  := sum ('=' sum)?
 *   sum   := term (('+'|'-') term)*
 *   term  := unary (('*'|'/') unary)*
 *   unary := ('+'|'-')* power
 *   power := atom ('^' unary)?      right-assoc: x^2^3 = x^(2^3)
 *   atom  := NUM | VAR | '(' sum ')'
 *
 * The equals sign is only allowed at the top level. The power rule
 * also accepts a unary exponent, so x^-1 and -x^2 (as -(x^2)) parse
 * as expected. */

static bool parse_sum(Parser *p, ToysPoly *out);
static bool parse_unary(Parser *p, ToysPoly *out);
static bool parse_power(Parser *p, ToysPoly *out);
static bool parse_atom(Parser *p, ToysPoly *out);

static bool parse_unary(Parser *p, ToysPoly *out)
{
    if (p->depth >= TOYS_EXPR_MAX_DEPTH) {
        err_set(p, p->pos, "expression too long");
        return false;
    }
    p->depth++;

    Token tok;
    lex_peek(p, &tok);
    bool ok = true;
    if (tok.type == TOYS_EXPR_OP && (tok.op == '+' || tok.op == '-')) {
        tok = lex_take(p);
        ToysPoly inner;
        if (!parse_unary(p, &inner))
            ok = false;
        else
            *out = (tok.op == '-') ? toys_poly_scale(&inner, -1.0) : inner;
    } else {
        ok = parse_power(p, out);
    }

    p->depth--;
    return ok;
}

static bool parse_power(Parser *p, ToysPoly *out)
{
    if (!parse_atom(p, out))
        return false;

    Token tok;
    lex_peek(p, &tok);
    if (tok.type == TOYS_EXPR_OP && tok.op == '^') {
        tok = lex_take(p);

        ToysPoly exponent;
        if (!parse_unary(p, &exponent))
            return false;

        double k = exponent.coeffs[0];
        if (exponent.degree != 0 || k < 0.0 || k != floor(k)) {
            err_set(p, tok.pos, "exponent must be a non-negative integer constant");
            return false;
        }
        if (k > 1048576.0) {
            err_set(p, tok.pos, "exponent too large");
            return false;
        }

        long long n = (long long)k;
        if (out->degree == 0) {
            /* constant base, 0^0 = 1 by convention */
            ToysPoly t = toys_poly_const(pow(out->coeffs[0], (double)n));
            if (!isfinite(t.coeffs[0])) {
                err_set(p, tok.pos, "coefficient overflows double");
                return false;
            }
            *out = t;
            return true;
        }

        ToysPoly result = toys_poly_const(1.0);
        for (long long i = 0; i < n; i++) {
            ToysPoly tmp;
            if (!toys_poly_mul(&result, out, &tmp)) {
                err_set(p, tok.pos, "result degree too large");
                return false;
            }
            result = tmp;
        }
        *out = result;
    }
    return true;
}

static bool parse_atom(Parser *p, ToysPoly *out)
{
    Token tok;
    lex_peek(p, &tok);

    if (tok.type == TOYS_EXPR_NUM || tok.type == TOYS_EXPR_VAR) {
        tok = lex_take(p);
        *out = (tok.type == TOYS_EXPR_NUM) ? toys_poly_const(tok.val)
                                           : toys_poly_x();
        return true;
    }

    if (tok.type == TOYS_EXPR_LPAREN) {
        tok = lex_take(p);
        if (!parse_sum(p, out))
            return false;
        tok = lex_take(p);
        if (tok.type != TOYS_EXPR_RPAREN) {
            err_set(p, tok.pos, "expected ')'");
            return false;
        }
        return true;
    }

    err_set(p, tok.pos,
            tok.type == TOYS_EXPR_END ? "unexpected end of expression"
                                      : "expected a number, x or '('");
    return false;
}

static bool parse_term(Parser *p, ToysPoly *out)
{
    if (!parse_unary(p, out))
        return false;

    Token tok;
    while (1) {
        lex_peek(p, &tok);
        if (tok.type != TOYS_EXPR_OP || (tok.op != '*' && tok.op != '/'))
            break;

        tok = lex_take(p);
        ToysPoly rhs;
        if (!parse_unary(p, &rhs))
            return false;

        if (tok.op == '*') {
            ToysPoly tmp;
            if (!toys_poly_mul(out, &rhs, &tmp)) {
                err_set(p, tok.pos, "result degree too large");
                return false;
            }
            *out = tmp;
        } else {
            if (rhs.degree != 0) {
                err_set(p, tok.pos, "division by x or a polynomial");
                return false;
            }
            if (rhs.coeffs[0] == 0.0) {
                err_set(p, tok.pos, "division by zero");
                return false;
            }
            *out = toys_poly_scale(out, 1.0 / rhs.coeffs[0]);
        }
    }
    return true;
}

static bool parse_sum(Parser *p, ToysPoly *out)
{
    if (!parse_term(p, out))
        return false;

    Token tok;
    while (1) {
        lex_peek(p, &tok);
        if (tok.type != TOYS_EXPR_OP || (tok.op != '+' && tok.op != '-'))
            break;

        tok = lex_take(p);
        ToysPoly rhs;
        if (!parse_term(p, &rhs))
            return false;

        *out = (tok.op == '+') ? toys_poly_add(out, &rhs)
                               : toys_poly_sub(out, &rhs);
    }
    return true;
}

static bool parse_expr(Parser *p, ToysPoly *out)
{
    if (!parse_sum(p, out))
        return false;

    Token tok;
    lex_peek(p, &tok);
    if (tok.type == TOYS_EXPR_EQ) {
        tok = lex_take(p);
        ToysPoly rhs;
        if (!parse_sum(p, &rhs))
            return false;
        *out = toys_poly_sub(out, &rhs);
    }
    return true;
}


const char *toys_expr_to_poly(const char *s, size_t len, ToysPoly *out,
                              size_t *err_pos)
{
    *err_pos = 0;

    Parser p = { 0 };
    p.s = s;
    p.len = len;
    p.err_pos = err_pos;

    if (!parse_expr(&p, out))
        return p.err_msg;

    Token tok;
    lex_peek(&p, &tok);
    if (tok.type != TOYS_EXPR_END) {
        err_set(&p, tok.pos, "unexpected token after expression");
        return p.err_msg;
    }

    /* Check for error that lex sets. */
    if (p.err_msg != NULL)
        return p.err_msg;

    toys_poly_trim(out);
    LOG_D("expr reduced to degree %d polynomial", out->degree);
    return NULL;
}
