#include "toys/debug.h"
#include "toys/math.h"
#include "toys/solve.h"

#include <ctype.h>
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
    EXPR_NUM,
    EXPR_VAR,
    EXPR_OP,
    EXPR_LPAREN,
    EXPR_RPAREN,
    // TOYS_EXPR_EQ,
    EXPR_END,
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
    /* Position where parser is stopped now, also used to store where error happened. */
    size_t pos;
    /* Depth of expr nesting at current pos. */
    size_t depth;
    /* Next token lookahead. */
    Token lookahead;
    /* Whether lookahead is already filled. */
    bool have;
    // /* [out] position in string at which first error happened. */
    // size_t *err_pos;
    // /* [out] error string. */
    // const char *err_msg;
} Parser;

// static void err_set(Parser *p, size_t pos, const char *msg)
// {
//     /* First error sticks. */
//     if (p->err_msg == NULL) {
//         p->err_msg = msg;
//         *p->err_pos = pos;
//     }
// }

/**
 * Reads a number token with strtod, then checks the consumed span
 * only holds digits, dot and e/E.
 * @param [inout] p parser state
 * @param [out] tok resulting number token on success
 * @return error string or `NULL` on success
 */
static const char *lex_number(Parser *p, Token *tok)
{
    assert(p != NULL);
    assert(tok != NULL);

    char *end = NULL;
    double val = strtod(p->s + p->pos, &end);
    size_t span = (size_t)(end - (p->s + p->pos));

    if (span == 0)
        return "failed to parse number";

    for (size_t i = 0; i < span; i++) {
        char c = p->s[p->pos + i];
        if (!((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E')) {
            // err_set(p, p->pos, "unexpected character");
            return "unexpected character in number";
        }
    }

    tok->type = EXPR_NUM;
    tok->pos = p->pos;
    tok->val = val;
    p->pos += span;
    return NULL;
}

/* Fills *tok with the next token, or returns error string. */
static const char *lex_next(Parser *p, Token *tok)
{
    assert(p != NULL);
    assert(tok != NULL);

    while (p->pos < p->len && isspace(p->s[p->pos]))
        p->pos++;

    tok->pos = p->pos;
    if (p->pos >= p->len) {
        tok->type = EXPR_END;
        return NULL;
    }

    char c = p->s[p->pos];
    switch (c) {
    case '0' ... '9':
    case '.':
        return lex_number(p, tok);
    case 'x':
        tok->type = EXPR_VAR;
        break;
    case '+': case '-':
    case '*': case '/':
    case '^':
        tok->type = EXPR_OP;
        tok->op = c;
        break;
    case '(':
        tok->type = EXPR_LPAREN;
        break;
    case ')':
        tok->type = EXPR_RPAREN;
        break;
    // case '=':
    //     tok->type = TOYS_EXPR_EQ;
    //     break;
    default:
        return "unexpected character";
    }
    p->pos++;
    return NULL;
}

static const char *lex_peek(Parser *p, Token *tok)
{
    assert(p != NULL);
    assert(tok != NULL);

    const char *ret = NULL;
    if (!p->have) {
        ret = lex_next(p, &p->lookahead);
        if (ret != NULL) {
            p->lookahead.type = EXPR_END;
            p->lookahead.pos = p->pos;
        }
        p->have = true;
    }
    *tok = p->lookahead;
    return ret;
}

static void lex_take(Parser *p)
{
    assert(p != NULL);

    p->have = false;
}

static const char *parse_sum(Parser *p, WstPoly *out);
static const char *parse_unary(Parser *p, WstPoly *out);

static const char *parse_atom(Parser *p, WstPoly *out)
{
    assert(p != NULL);
    assert(out != NULL);

    Token tok;
    const char *ret = NULL;
    ret = lex_peek(p, &tok);
    if (ret != NULL)
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
        parse_sum(p, out);
        lex_peek(p, &tok);
        if (tok.type != EXPR_RPAREN)
            return "expected ')'";
        lex_take(p);
        break;
    case EXPR_END:
        return "unexpected end of expression";
    default:
        return "expected number, x or '('";
    }

    return NULL;
}

static const char *parse_power(Parser *p, WstPoly *out)
{
    assert(p != NULL);
    assert(out != NULL);

    const char *ret = parse_atom(p, out);
    if (ret != NULL) return ret;

    Token tok;
    lex_peek(p, &tok);
    if (tok.type != EXPR_OP || tok.op != '^')
        return NULL;
    lex_take(p);
    WstPoly e = { 0 };
    ret = parse_unary(p, &e);
    if (ret != NULL) return ret;

    // TODO: better checks
    if (e.degree != 0 || !my_iszero(floor(e.coeffs[0]) - e.coeffs[0]))
        return "Power must be a constant non-negative integer";
    int pow = (int)e.coeffs[0];
    if (pow < 0)
        return "Power must be a constant non-negative integer";

    WstPoly src = *out;
    memset(out, 0, sizeof(*out));
    out->coeffs[0] = 1.0;

    for (int i = 0; i < pow; i++) {
        if (!wst_poly_mul(out, &src))
            return "Polynomial capacity exceeded";
    }
    return NULL;
}

static const char *parse_unary(Parser *p, WstPoly *out)
{
    assert(p != NULL);
    assert(out != NULL);

    bool neg = false;
    while (1) {
        Token tok;
        lex_peek(p, &tok);
        if (tok.type != EXPR_OP || (tok.op != '+' && tok.op != '-'))
            break;
        lex_take(p);
        if (tok.op == '-')
            neg = !neg;
    }
    const char *ret = parse_power(p, out);
    if (ret != NULL) return ret;

    if (neg)
        wst_poly_scale(out, -1.0);
    return NULL;
}

static const char *parse_mul(Parser *p, WstPoly *out)
{
    assert(p != NULL);
    assert(out != NULL);

    const char *ret = parse_unary(p, out);
    if (ret != NULL) return ret;

    while (1) {
        Token tok;
        lex_peek(p, &tok);
        if (tok.type != EXPR_OP || (tok.op != '*' && tok.op != '/'))
            break;
        lex_take(p);
        WstPoly rhs = { 0 };
        ret = parse_unary(p, &rhs);
        if (ret != NULL) return ret;
    
        if (tok.op == '/') {
            if (rhs.degree != 0 || my_iszero(rhs.coeffs[0]))
                return "Cannot divide by non-constant or zero";
            wst_poly_scale(out, 1.0 / rhs.coeffs[0]);
        }
        else if (!wst_poly_mul(out, &rhs))
            return "Polynomial capacity exceeded";
    }

    return NULL;
}

static const char *parse_sum(Parser *p, WstPoly *out)
{
    const char *ret = parse_mul(p, out);
    if (ret != NULL) return ret;

    while (1) {
        Token tok;
        lex_peek(p, &tok);
        if (tok.type != EXPR_OP || (tok.op != '+' && tok.op != '-'))
            break;
        lex_take(p);
        WstPoly rhs = { 0 };
        ret = parse_mul(p, &rhs);
        if (ret != NULL) return ret;

        if (tok.op == '+')
            wst_poly_add(out, &rhs);
        else
            wst_poly_sub(out, &rhs);
    }

    return NULL;
}

const char *wst_expr_to_poly(const char *s, size_t len, WstPoly *out, size_t *err_pos)
{
    Parser p = { .s = s, .len = len };
    const char *ret = parse_sum(&p, out);
    *err_pos = p.pos;
    return ret;
}
