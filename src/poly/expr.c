#include "toys/debug.h"
#include "toys/math.h"
#include "toys/poly.h"
#include "toys/expr.h"

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Nesting cap for '(' and '-' chains, which recurse per character. */
/* TODO: this is unused and unchecked */
// #define WST_EXPR_MAX_DEPTH 256

/** Types of tokens that lexer can parse. */
typedef enum {
    EXPR_NUM,
    EXPR_VAR,
    EXPR_OP,
    EXPR_LPAREN,
    EXPR_RPAREN,
    // EXPR_EQ,
    EXPR_END,
} TokenType;

/** Token characters */
typedef enum {
    EXPR_TOK_LPAREN = '(',
    EXPR_TOK_RPAREN = ')',
    EXPR_TOK_MUL    = '*',
    EXPR_TOK_ADD    = '+',
    EXPR_TOK_SUB    = '-',
    EXPR_TOK_DIV    = '/',
    EXPR_TOK_POW    = '^',
    EXPR_TOK_X      = 'x',
} TokenOp;

/** Single expression token. */
typedef struct {
    TokenType type;
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
    /* Position where parser is stopped now, also used to store where error happened. */
    size_t pos;
    /* Depth of expr nesting at current pos. */
    // size_t depth;
    /* Next token lookahead. */
    Token lookahead;
    /* Whether lookahead is already filled. */
    bool have;
} Parser;

const char *wst_expr_err_string(WstParserErr err)
{
    static const char *parser_err[] = {
        [WST_EXPR_NO_ERR] = "no error",
        [WST_EXPR_FAILED_TO_PARSE_NUMBER]  = "failed to parse number",
        [WST_EXPR_UNEXPECTED_CHAR_IN_NUM]  = "unexpected character in number",
        [WST_EXPR_UNEXPECTED_CHAR_IN_EXPR] = "unexpected character in expression",
        [WST_EXPR_UNEXPECTED_END_OF_EXPR]  = "unexpected end of expression",
        [WST_EXPR_UNEXPECTED_ATOM]         = "expected number, x or '('",
        [WST_EXPR_NON_INTEGER_POWER]       = "power must be a constant non-negative integer",
        [WST_EXPR_DEGREE_EXCEEDED]         = "polynomial degree capacity exceeded",
        [WST_EXPR_DIV_ERR]                 = "cannot divide by non-constant or zero",
        [WST_EXPR_MISSING_RPAREN]          = "missing ')'",
    };
    return parser_err[err];
}

/* Parse string with raw polynomial coefficients in ascending order separated by spaces. */
/* TODO: combine number parsing for both modes into one routine */

static const char *skip_seps(const char *s)
{
    while (isspace(*s)) s++;
    return s;
}

static const char *token_end(const char *s)
{
    while (!isspace(*s) && *s != '\0') s++;
    return s;
}

static bool parse_coeff(const char *str, size_t len, double *out)
{
    assert(out != NULL);

    char *end = NULL;
    double val = strtod(str, &end);
    if (end == str || end != str + len)
        return false;
    *out = val;
    return true;
}

/** Parse whitespace separated polynomial coeffs in ascending order. */
static WstParserErr parse_poly_coeffs(const char *line, WstPoly *poly, size_t *err_pos)
{
    assert(line != NULL);
    assert(poly != NULL);

    int n = 0;
    for (const char *p = skip_seps(line); *p != '\0'; p = skip_seps(token_end(p)))
        n++;
    if (n == 0) {
        *err_pos = 0;
        return WST_EXPR_UNEXPECTED_END_OF_EXPR;
    }
    if (n - 1 > WST_POLY_MAX_DEGREE) {
        *err_pos = strlen(line) - 1; /* Could be more accurate */
        return WST_EXPR_DEGREE_EXCEEDED;
    }

    int i = 0;
    for (const char *p = skip_seps(line); *p != '\0';) {
        const char *end = token_end(p);
        if (!parse_coeff(p, (size_t)(end - p), &poly->coeffs[i])) {
            *err_pos = (size_t)(p - line);
            return WST_EXPR_FAILED_TO_PARSE_NUMBER;
        }
        poly->degree = i;
        i++;
        p = skip_seps(end);
    }
    return WST_EXPR_NO_ERR;
}

/**
 * Reads a number token with strtod, then checks the consumed span
 * only holds digits, dot and [+-]e/E.
 * TODO: finite state machine can be used in some places like number parsing.
 *
 * @param [inout] p parser state
 * @param [out] tok resulting number token on success
 * @return error string or `NULL` on success
 */
static WstParserErr lex_number(Parser *p, Token *tok)
{
    assert(p != NULL);
    assert(tok != NULL);

    const char *start = p->s + p->pos;
    char *end = NULL;

    double val = strtod(start, &end);
    ptrdiff_t span = end - start;

    if (span <= 0) {
        return WST_EXPR_FAILED_TO_PARSE_NUMBER;
    }

    for (ptrdiff_t i = 0; i < span; i++) {
        char c = start[i];
        if (!((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-')) {
            return WST_EXPR_UNEXPECTED_CHAR_IN_NUM;
        }
    }

    tok->type = EXPR_NUM;
    tok->val = val;
    p->pos += (size_t)span;

    return WST_EXPR_NO_ERR;
}

/* Fills *tok with the next token, or returns error string. */
static WstParserErr lex_next(Parser *p, Token *tok)
{
    assert(p != NULL);
    assert(tok != NULL);

    while (p->pos < p->len && isspace(p->s[p->pos]))
        p->pos++;

    if (p->pos >= p->len) {
        tok->type = EXPR_END;
        return WST_EXPR_NO_ERR;
    }

    char c = p->s[p->pos];
    switch (c) {
    case '0' ... '9':
    case '.':
        return lex_number(p, tok);
    case EXPR_TOK_X:
        tok->type = EXPR_VAR;
        break;
    case EXPR_TOK_ADD: case EXPR_TOK_SUB:
    case EXPR_TOK_MUL: case EXPR_TOK_DIV:
    case EXPR_TOK_POW:
        tok->type = EXPR_OP;
        tok->op = c;
        break;
    case EXPR_TOK_LPAREN:
        tok->type = EXPR_LPAREN;
        break;
    case EXPR_TOK_RPAREN:
        tok->type = EXPR_RPAREN;
        break;
    // case '=':
    //     tok->type = TOYS_EXPR_EQ;
    //     break;
    default:
        return WST_EXPR_UNEXPECTED_CHAR_IN_EXPR;
    }
    p->pos++;
    return WST_EXPR_NO_ERR;
}

/** Peeks at next token in string without moving forward. */
static WstParserErr lex_peek(Parser *p, Token *tok)
{
    assert(p != NULL);
    assert(tok != NULL);

    WstParserErr ret = WST_EXPR_NO_ERR;
    if (!p->have) {
        ret = lex_next(p, &p->lookahead);
        if (ret != WST_EXPR_NO_ERR) {
            LOG_D("lex_next: %s", wst_expr_err_string(ret));
            p->lookahead.type = EXPR_END;
        }
        p->have = true;
    }
    *tok = p->lookahead;
    return ret;
}

/** Marks current token work as complete if it was started with `lex_peek`. */
static void lex_take(Parser *p)
{
    assert(p != NULL);

    p->have = false;
}

static WstParserErr parse_expr(Parser *p, WstPoly *out);
static WstParserErr parse_unary(Parser *p, WstPoly *out);

static WstParserErr parse_atom(Parser *p, WstPoly *out)
{
    assert(p != NULL);
    assert(out != NULL);

    Token tok;
    WstParserErr ret = WST_EXPR_NO_ERR;
    ret = lex_peek(p, &tok);
    if (ret != WST_EXPR_NO_ERR)
        return ret;

    switch (tok.type) {
    case EXPR_NUM:
        lex_take(p);
        memset(out, 0, sizeof(*out));
        out->coeffs[0] = tok.val;
        break;
    case EXPR_VAR:
        lex_take(p);
        memset(out, 0, sizeof(*out));
        out->degree = 1;
        out->coeffs[1] = 1.0;
        break;
    case EXPR_LPAREN:
        lex_take(p);
        parse_expr(p, out);
        ret = lex_peek(p, &tok);
        if (ret != WST_EXPR_NO_ERR)
            return ret;
        if (tok.type != EXPR_RPAREN)
            return WST_EXPR_MISSING_RPAREN;
        lex_take(p);
        break;
    case EXPR_END:
        return WST_EXPR_UNEXPECTED_END_OF_EXPR;
    default:
        return WST_EXPR_UNEXPECTED_ATOM;
    }

    return ret;
}

static WstParserErr parse_power(Parser *p, WstPoly *out)
{
    assert(p != NULL);
    assert(out != NULL);

    WstParserErr ret = parse_atom(p, out);
    if (ret != WST_EXPR_NO_ERR) return ret;

    Token tok;
    ret = lex_peek(p, &tok);
    if (ret != WST_EXPR_NO_ERR) return ret;
    if (tok.type != EXPR_OP || tok.op != EXPR_TOK_POW)
        return WST_EXPR_NO_ERR;
    lex_take(p);
    WstPoly e = { 0 };
    ret = parse_unary(p, &e);
    if (ret != WST_EXPR_NO_ERR) return ret;

    // TODO: better checks
    if (e.degree != 0 || !my_iszero(floor(e.coeffs[0]) - e.coeffs[0]))
        return WST_EXPR_NON_INTEGER_POWER;
    int pow = (int)e.coeffs[0];
    if (pow < 0)
        return WST_EXPR_NON_INTEGER_POWER;

    WstPoly src = *out;
    memset(out, 0, sizeof(*out));
    out->coeffs[0] = 1.0;

    for (int i = 0; i < pow; i++) {
        if (!wst_poly_mul(out, &src))
            return WST_EXPR_DEGREE_EXCEEDED;
    }
    return ret;
}

static WstParserErr parse_unary(Parser *p, WstPoly *out)
{
    assert(p != NULL);
    assert(out != NULL);

    WstParserErr ret;

    bool neg = false;
    while (1) {
        Token tok;
        ret = lex_peek(p, &tok);
        if (ret != WST_EXPR_NO_ERR) return ret;
        if (tok.type != EXPR_OP || (tok.op != EXPR_TOK_ADD && tok.op != EXPR_TOK_SUB))
            break;
        lex_take(p);
        if (tok.op == EXPR_TOK_SUB)
            neg = !neg;
    }
    ret = parse_power(p, out);
    if (ret != WST_EXPR_NO_ERR) return ret;

    if (neg)
        wst_poly_scale(out, -1.0);
    return ret;
}

static WstParserErr parse_term(Parser *p, WstPoly *out)
{
    assert(p != NULL);
    assert(out != NULL);

    WstParserErr ret = parse_unary(p, out);
    if (ret != WST_EXPR_NO_ERR) return ret;

    while (1) {
        Token tok;
        ret = lex_peek(p, &tok);
        if (ret != WST_EXPR_NO_ERR) return ret;
        if (tok.type != EXPR_OP || (tok.op != EXPR_TOK_MUL && tok.op != EXPR_TOK_DIV))
            break;
        lex_take(p);
        WstPoly rhs = { 0 };
        ret = parse_unary(p, &rhs);
        if (ret != WST_EXPR_NO_ERR) return ret;
    
        if (tok.op == EXPR_TOK_DIV) {
            if (rhs.degree != 0 || my_iszero(rhs.coeffs[0]))
                return WST_EXPR_DIV_ERR;
            wst_poly_scale(out, 1.0 / rhs.coeffs[0]);
        }
        else if (!wst_poly_mul(out, &rhs))
            return WST_EXPR_DEGREE_EXCEEDED;
    }

    return ret;
}

static WstParserErr parse_expr(Parser *p, WstPoly *out)
{
    assert(p != NULL);
    assert(out != NULL);

    WstParserErr ret = parse_term(p, out);
    if (ret != WST_EXPR_NO_ERR) return ret;

    while (1) {
        Token tok;
        ret = lex_peek(p, &tok);
        if (ret != WST_EXPR_NO_ERR) return ret;
        if (tok.type != EXPR_OP || (tok.op != EXPR_TOK_ADD && tok.op != EXPR_TOK_SUB))
            break;
        lex_take(p);
        WstPoly rhs = { 0 };
        ret = parse_term(p, &rhs);
        if (ret != WST_EXPR_NO_ERR) return ret;

        if (tok.op == EXPR_TOK_ADD)
            wst_poly_add(out, &rhs);
        else
            wst_poly_sub(out, &rhs);
    }

    return ret;
}

WstParserErr wst_expr_to_poly(const char *s, WstPoly *out, size_t *err_pos, bool expr_mode)
{
    assert(s != NULL);
    assert(out != NULL);

    WstParserErr ret;

    if (!expr_mode) {
        ret = parse_poly_coeffs(s, out, err_pos);
        return ret;
    }

    Parser p = { .s = s, .len = strlen(s), };
    ret = parse_expr(&p, out);
    if (ret != WST_EXPR_NO_ERR) {
        if (err_pos != NULL)
            *err_pos = p.pos;
        return ret;
    }

    Token tok;
    ret = lex_peek(&p, &tok);
    if (ret != WST_EXPR_NO_ERR) {
        if (err_pos != NULL)
            *err_pos = p.pos;
        return ret;
    }
    if (tok.type != EXPR_END) {
        if (err_pos != NULL)
            *err_pos = p.pos;
        return WST_EXPR_UNEXPECTED_ATOM;
    }
    return WST_EXPR_NO_ERR;
}
