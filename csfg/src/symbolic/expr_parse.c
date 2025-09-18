#include "csfg/symbolic/expr.h"
#include <ctype.h>
#include <stdint.h>

#define CHAR_TOKEN_LIST                                                        \
    X(ADD, '+')                                                                \
    X(SUB, '-')                                                                \
    X(MUL, '*')                                                                \
    X(DIV, '/')                                                                \
    X(POW, '^')                                                                \
    X(LB, '(')                                                                 \
    X(RB, ')')

enum token
{
    TOK_ERROR = -1,
    TOK_END = 0,
#define X(name, char) TOK_##name = char,
    CHAR_TOKEN_LIST
#undef X
        TOK_LIT = 256,
    TOK_IDENT,
};

struct parser
{
    const char* text;
    union
    {
        double         lit;
        struct strspan var;
    } value;
    int        head, tail, end;
    enum token tok;
};

/* ------------------------------------------------------------------------- */
static void parser_init(struct parser* p, const char* text)
{
    p->text = text;
    p->end = strlen(text);
    p->head = 0;
    p->tail = 0;
    p->tok = TOK_ERROR;
}

/* ------------------------------------------------------------------------- */
static enum token scan_next(struct parser* p)
{
    p->tail = p->head;
    while (p->head != p->end)
    {
        /* Literal value */
        if (isdigit(p->text[p->head]) || p->text[p->head] == '-')
        {
            char is_neg = p->text[p->head] == '-';
            if (p->text[p->head] == '-')
                p->head++;

            p->value.lit = 0;
            for (; p->head != p->end && isdigit(p->text[p->head]); ++p->head)
            {
                p->value.lit *= 10;
                p->value.lit += p->text[p->head] - '0';
            }
            /* It is actually a float */
            if (p->head != p->end && p->text[p->head] == '.')
            {
                double fraction = 1.0;
                for (p->head++; p->head != p->end && isdigit(p->text[p->head]);
                     ++p->head)
                {
                    fraction *= 0.1;
                    p->value.lit += fraction * (double)(p->text[p->head] - '0');
                }
                if (p->head != p->end && p->text[p->head] == 'f')
                    ++p->head;
            }

            if (is_neg)
                p->value.lit = -p->value.lit;
            return p->tok = TOK_LIT;
        }

        /* Identifier [a-zA-Z_][a-zA-Z0-9_]* */
        if (isalpha(p->text[p->head]) || p->text[p->head] == '_')
        {
            p->value.var.off = p->head++;
            while (p->head != p->end &&
                   (isalnum(p->text[p->head]) || p->text[p->head] == '_'))
            {
                p->head++;
            }
            p->value.var.len = p->head - p->value.var.off;
            return p->tok = TOK_IDENT;
        }

        /* Single characters */
        switch (p->text[p->head])
        {
#define X(name, char)                                                          \
    case char: return p->tok = p->text[p->head++];
            CHAR_TOKEN_LIST
#undef X
            case ' ':
            case '\t':
            case '\n':
            case '\r': p->tail = ++p->head; break;
            default: return p->tok = TOK_ERROR;
        }
    }

    return p->tok = TOK_END;
}

/* ------------------------------------------------------------------------- */
static int parse_expr(struct parser* p, struct csfg_expr** e);
static int parse_base(struct parser* p, struct csfg_expr** e)
{
    int n;

    switch (p->tok)
    {
        case TOK_LIT:
            n = csfg_expr_lit(e, p->value.lit);
            scan_next(p);
            return n;

        case TOK_IDENT:
            n = csfg_expr_var(
                e, strview(p->text, p->value.var.off, p->value.var.len));
            scan_next(p);
            return n;

        case '(': {
            scan_next(p);
            n = parse_expr(p, e);
            if (n == -1)
                return -1;
            if (p->tok != ')')
                return -1;
            scan_next(p);
            return n;
        }

        default: return -1;
    }

    return -1;
}

/* ------------------------------------------------------------------------- */
static int parse_exponent(struct parser* p, struct csfg_expr** e)
{
    int sign = 1;

    while (p->tok == '+' || p->tok == '-')
    {
        if (p->tok == '-')
            sign = -sign;
        scan_next(p);
    }

    if (sign == 1)
        return parse_base(p, e);
    else
    {
        int zero = csfg_expr_lit(e, 0.0);
        int expr = parse_base(p, e);
        if (zero == -1 || expr == -1)
            return -1;
        return csfg_expr_sub(e, zero, expr);
    }
}

/* ------------------------------------------------------------------------- */
static int parse_factor(struct parser* p, struct csfg_expr** e)
{
    int child, pow = parse_exponent(p, e);
    if (pow == -1)
        return -1;

    while (p->tok == '^')
    {
        scan_next(p);
        child = parse_exponent(p, e);
        if (child == -1)
            return -1;
        pow = csfg_expr_pow(e, pow, child);
        if (pow == -1)
            return -1;
    }

    return pow;
}

/* ------------------------------------------------------------------------- */
static int parse_term(struct parser* p, struct csfg_expr** e)
{
    int child, factor = parse_factor(p, e);
    if (factor == -1)
        return -1;

    while (p->tok == TOK_MUL || p->tok == TOK_DIV)
    {
        enum csfg_expr_type op =
            p->tok == TOK_MUL ? CSFG_EXPR_OP_MUL : CSFG_EXPR_OP_DIV;

        scan_next(p);
        child = parse_factor(p, e);
        if (child == -1)
            return -1;
        factor = csfg_expr_binop(e, op, factor, child);
        if (factor == -1)
            return -1;
    }

    return factor;
}

/* ------------------------------------------------------------------------- */
static int parse_expr(struct parser* p, struct csfg_expr** e)
{
    int child, term = parse_term(p, e);
    if (term == -1)
        return -1;

    while (p->tok == TOK_ADD || p->tok == TOK_SUB)
    {
        enum csfg_expr_type op =
            p->tok == TOK_ADD ? CSFG_EXPR_OP_ADD : CSFG_EXPR_OP_SUB;

        scan_next(p);
        child = parse_term(p, e);
        if (child == -1)
            return -1;
        term = csfg_expr_binop(e, op, term, child);
        if (term == -1)
            return -1;
    }

    return term;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_parse(struct csfg_expr** expr, const char* text)
{
    int           root;
    struct parser p;
    parser_init(&p, text);

    scan_next(&p);
    root = parse_expr(&p, expr);
    if (root == -1)
    {
        union
        {
            int16_t offlen[2];
            int     value;
        } error;
        error.offlen[0] = p.tail;
        error.offlen[1] = p.head - p.tail;
        return error.value;
    }

    (*expr)->root = root;

    return 0;
}
