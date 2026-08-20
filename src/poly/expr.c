#include "toys/debug.h"
#include "toys/expr.h"
#include "poly_impl.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


/* Every token becomes at most one tree node, so one budget bounds both
 * the lexer and the parser's fixed node pool. */
#define TOYS_EXPR_MAX_TOKENS 256

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

typedef struct Node {
    Token token;
    struct Node *left;
    struct Node *right;
} Node;

typedef struct {
    size_t *err_pos;
    const char **err_msg;
} Err;

/* Only the first error sticks, so later ones can't override a better message. */
static void err_set(Err *e, size_t pos, const char *msg)
{
    if (*e->err_msg == NULL) {
        *e->err_pos = pos;
        *e->err_msg = msg;
    }
}


/* Lexer */

typedef struct {
    const char *s;
    size_t len;
    size_t pos;
    Token lookahead;
    int have;
    int count;
} Lexer;

/* strtod over the token start; the consumed span must only contain
 * [0-9.eE], so lexemes like "inf" or "nan" are rejected. */
static int lex_number(Lexer *lx, Token *tok, Err *e)
{
    char *end = NULL;
    double val = strtod(lx->s + lx->pos, &end);
    size_t span = (size_t)(end - (lx->s + lx->pos));

    if (span == 0)
        return 0;

    for (size_t i = 0; i < span; i++) {
        char c = lx->s[lx->pos + i];
        if (!((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E')) {
            err_set(e, lx->pos, "unexpected character");
            return 0;
        }
    }

    tok->type = TOYS_EXPR_NUM;
    tok->pos = lx->pos;
    tok->val = val;
    lx->pos += span;
    return 1;
}

/* Fills *tok with the next token; returns 0 on an unexpected character. */
static int lex_next(Lexer *lx, Token *tok, Err *e)
{
    if (lx->have) {
        *tok = lx->lookahead;
        lx->have = 0;
        return 1;
    }

    while (lx->pos < lx->len) {
        char c = lx->s[lx->pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            lx->pos++;
        else
            break;
    }

    tok->pos = lx->pos;
    if (lx->pos >= lx->len) {
        tok->type = TOYS_EXPR_END;
        return 1;
    }

    char c = lx->s[lx->pos];
    if ((c >= '0' && c <= '9') || c == '.')
        return lex_number(lx, tok, e);

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
        err_set(e, lx->pos, "unexpected character");
        return 0;
    }
    lx->pos++;

    if (++lx->count > TOYS_EXPR_MAX_TOKENS) {
        err_set(e, tok->pos, "expression too long");
        return 0;
    }
    return 1;
}


/* Parser: recursive descent, nodes come from a fixed pool.
 *
 *   expr  := sum ('=' sum)?
 *   sum   := term (('+'|'-') term)*
 *   term  := unary (('*'|'/') unary)*
 *   unary := ('+'|'-')* power
 *   power := atom ('^' unary)?      right-assoc: x^2^3 = x^(2^3)
 *   atom  := NUM | VAR | '(' expr ')'
 *
 * "=" is only allowed at the top level; the power rule also accepts a
 * unary exponent, so x^-1 and -x^2 (as -(x^2)) parse as expected. */

typedef struct {
    Lexer lx;
    Err err;
    Node pool[TOYS_EXPR_MAX_TOKENS];
    int npool;
} Parser;

static void lex_peek(Parser *p, Token *tok)
{
    if (!p->lx.have) {
        if (!lex_next(&p->lx, &p->lx.lookahead, &p->err)) {
            p->lx.lookahead.type = TOYS_EXPR_END;
            p->lx.lookahead.pos = p->lx.pos;
        }
        p->lx.have = 1;
    }
    *tok = p->lx.lookahead;
}

static Token lex_take(Parser *p)
{
    Token tok;
    lex_peek(p, &tok);
    p->lx.have = 0;
    return tok;
}

static Node *new_node(Parser *p, Token tok)
{
    if (p->npool >= TOYS_EXPR_MAX_TOKENS) {
        err_set(&p->err, tok.pos, "expression too long");
        return NULL;
    }

    Node *n = &p->pool[p->npool++];
    n->token = tok;
    n->left = NULL;
    n->right = NULL;
    return n;
}

static Node *new_unary(Parser *p, Token op, Node *operand)
{
    Node *n = new_node(p, op);
    if (n)
        n->right = operand;
    return n;
}

static Node *new_binary(Parser *p, Token op, Node *left, Node *right)
{
    Node *n = new_node(p, op);
    if (n) {
        n->left = left;
        n->right = right;
    }
    return n;
}

static Node *parse_sum(Parser *p);
static Node *parse_atom(Parser *p);
static Node *parse_power(Parser *p);

static Node *parse_unary(Parser *p)
{
    Token tok;
    lex_peek(p, &tok);
    if (tok.type == TOYS_EXPR_OP && (tok.op == '+' || tok.op == '-')) {
        tok = lex_take(p);
        Node *n = parse_unary(p);
        if (!n)
            return NULL;
        return new_unary(p, tok, n);
    }
    return parse_power(p);
}

static Node *parse_power(Parser *p)
{
    Node *n = parse_atom(p);
    Token tok;
    if (n) {
        lex_peek(p, &tok);
        if (tok.type == TOYS_EXPR_OP && tok.op == '^') {
            tok = lex_take(p);
            Node *exponent = parse_unary(p);
            if (!exponent)
                return NULL;
            n = new_binary(p, tok, n, exponent);
        }
    }
    return n;
}

static Node *parse_term(Parser *p)
{
    Node *n = parse_unary(p);
    Token tok;
    while (n) {
        lex_peek(p, &tok);
        if (tok.type != TOYS_EXPR_OP || (tok.op != '*' && tok.op != '/'))
            break;
        tok = lex_take(p);
        Node *rhs = parse_unary(p);
        if (!rhs)
            return NULL;
        n = new_binary(p, tok, n, rhs);
    }
    return n;
}

static Node *parse_sum(Parser *p)
{
    Node *n = parse_term(p);
    Token tok;
    while (n) {
        lex_peek(p, &tok);
        if (tok.type != TOYS_EXPR_OP || (tok.op != '+' && tok.op != '-'))
            break;
        tok = lex_take(p);
        Node *rhs = parse_term(p);
        if (!rhs)
            return NULL;
        n = new_binary(p, tok, n, rhs);
    }
    return n;
}

static Node *parse_expr(Parser *p)
{
    Node *n = parse_sum(p);
    Token tok;
    if (n) {
        lex_peek(p, &tok);
        if (tok.type == TOYS_EXPR_EQ) {
            tok = lex_take(p);
            Node *rhs = parse_sum(p);
            if (!rhs)
                return NULL;
            n = new_binary(p, tok, n, rhs);
        }
    }
    return n;
}

static Node *parse_atom(Parser *p)
{
    Token tok;
    lex_peek(p, &tok);

    if (tok.type == TOYS_EXPR_NUM || tok.type == TOYS_EXPR_VAR) {
        tok = lex_take(p);
        return new_node(p, tok);
    }

    if (tok.type == TOYS_EXPR_LPAREN) {
        tok = lex_take(p);
        Node *inner = parse_sum(p);
        if (!inner)
            return NULL;
        tok = lex_take(p);
        if (tok.type != TOYS_EXPR_RPAREN) {
            err_set(&p->err, tok.pos, "expected ')'");
            return NULL;
        }
        return inner;
    }

    err_set(&p->err, tok.pos,
            tok.type == TOYS_EXPR_END ? "unexpected end of expression"
                                      : "expected a number, x or '('");
    return NULL;
}


/* Evaluation */

static int eval_node(const Node *n, ToysPoly *out, Err *e)
{
    switch (n->token.type) {
    case TOYS_EXPR_NUM:
        *out = toys_poly_const(n->token.val);
        return 1;
    case TOYS_EXPR_VAR:
        *out = toys_poly_x();
        return 1;
    case TOYS_EXPR_EQ: {
        ToysPoly left, right;
        if (!eval_node(n->left, &left, e) || !eval_node(n->right, &right, e))
            return 0;
        *out = toys_poly_sub(&left, &right);
        return 1;
    }
    case TOYS_EXPR_OP: {
        if (n->left == NULL) {
            /* unary operation */
            if (!eval_node(n->right, out, e))
                return 0;
            if (n->token.op == '-')
                *out = toys_poly_scale(out, -1.0);
            return 1;
        }

        ToysPoly left, right;
        if (!eval_node(n->left, &left, e) || !eval_node(n->right, &right, e))
            return 0;

        switch (n->token.op) {
        case '+':
            *out = toys_poly_add(&left, &right);
            return 1;
        case '-':
            *out = toys_poly_sub(&left, &right);
            return 1;
        case '*':
            if (!toys_poly_mul(&left, &right, out)) {
                err_set(e, n->token.pos, "result degree exceeds 64");
                return 0;
            }
            return 1;
        case '/':
            if (right.degree != 0) {
                err_set(e, n->token.pos, "division by x or a polynomial");
                return 0;
            }
            if (right.coeffs[0] == 0.0) {
                err_set(e, n->token.pos, "division by zero");
                return 0;
            }
            *out = toys_poly_scale(&left, 1.0 / right.coeffs[0]);
            return 1;
        case '^': {
            double exponent = right.coeffs[0];
            if (right.degree != 0 || exponent < 0.0 || exponent != floor(exponent)) {
                err_set(e, n->token.pos,
                        "exponent must be a non-negative integer constant");
                return 0;
            }
            if (exponent > 1048576.0) {
                err_set(e, n->token.pos, "exponent too large");
                return 0;
            }

            long long k = (long long)exponent;
            if (left.degree == 0) {
                /* constant base, 0^0 = 1 by convention */
                *out = toys_poly_const(pow(left.coeffs[0], (double)k));
                if (!isfinite(out->coeffs[0])) {
                    err_set(e, n->token.pos, "coefficient overflows double");
                    return 0;
                }
                return 1;
            }

            *out = toys_poly_const(1.0);
            for (long long i = 0; i < k; i++) {
                ToysPoly tmp;
                if (!toys_poly_mul(out, &left, &tmp)) {
                    err_set(e, n->token.pos, "result degree exceeds 64");
                    return 0;
                }
                *out = tmp;
            }
            return 1;
        }
        default:
            err_set(e, n->token.pos, "unsupported operator");
            return 0;
        }
    }
    case TOYS_EXPR_LPAREN:
    case TOYS_EXPR_RPAREN:
    case TOYS_EXPR_END:
        err_set(e, n->token.pos, "internal parse error");
        return 0;
    }
    return 0;
}

int toys_expr_to_poly(const char *s, size_t len, ToysPoly *out,
                      size_t *err_pos, const char **err_msg)
{
    *err_pos = 0;
    *err_msg = NULL;

    Parser p = { 0 };
    p.lx.s = s;
    p.lx.len = len;
    p.err.err_pos = err_pos;
    p.err.err_msg = err_msg;

    Node *root = parse_expr(&p);
    if (!root)
        return 0;

    Token tok;
    lex_peek(&p, &tok);
    if (tok.type != TOYS_EXPR_END) {
        err_set(&p.err, tok.pos, "unexpected token after expression");
        return 0;
    }

    /* A lexing error sets err_msg but lets the parser finish on the END
     * sentinel, so check it explicitly. */
    if (*err_msg != NULL)
        return 0;

    if (!eval_node(root, out, &p.err))
        return 0;

    toys_poly_trim(out);
    LOG_D("expr reduced to degree %d polynomial", out->degree);
    return 1;
}
