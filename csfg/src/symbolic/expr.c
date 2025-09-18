#include "csfg/symbolic/expr.h"
#include "csfg/util/mem.h"
#include "csfg/util/strspan.h"
#include <ctype.h>
#include <stddef.h>
#include <string.h>

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
        struct strspan ident;
    } value;
    int head, tail, end;
};

/* ------------------------------------------------------------------------- */
static void parser_init(struct parser* p, const char* text)
{
    p->text = text;
    p->end = strlen(text);
    p->head = 0;
    p->tail = 0;
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
            return TOK_LIT;
        }

        /* Identifier [a-zA-Z_-][a-zA-Z0-9_-]* */
        if (isalpha(p->text[p->head]) || p->text[p->head] == '_' ||
            p->text[p->head] == '-')
        {
            p->value.ident.off = p->head++;
            while (p->head != p->end &&
                   (isalnum(p->text[p->head]) || p->text[p->head] == '_' ||
                    p->text[p->head] == '-'))
            {
                p->head++;
            }
            p->value.ident.len = p->head - p->value.ident.off;
            return TOK_IDENT;
        }

        /* Single characters */
        switch (p->text[p->head])
        {
#define X(name, char)                                                          \
    case char: return p->text[p->head++];
            CHAR_TOKEN_LIST
#undef X
            case ' ':
            case '\t':
            case '\n':
            case '\r': p->tail = ++p->head; break;
            default: return TOK_ERROR;
        }
    }

    return TOK_END;
}

/* ------------------------------------------------------------------------- */
static int parse_exponent(struct parser* p, struct csfg_expr** e)
{
}

/* ------------------------------------------------------------------------- */
static int parse_power(struct parser* p, struct csfg_expr** e)
{
    int sign = 1;

    enum token tok = scan_next(p);
    while (tok == '+' || tok == '-')
    {
        if (tok == '-')
            sign = -sign;
        tok = scan_next(p);
    }
}

/* ------------------------------------------------------------------------- */
static int parse_factor(struct parser* p, struct csfg_expr** e)
{
    if (parse_power(p, e) != 0)
        return -1;
}

/* ------------------------------------------------------------------------- */
static int parse_term(struct parser* p, struct csfg_expr** e)
{
    if (parse_factor(p, e) != 0)
        return -1;
}

/* ------------------------------------------------------------------------- */
static int parse_expr(struct parser* p, struct csfg_expr** e)
{
    if (parse_term(p, e) != 0)
        return -1;
}

/* ------------------------------------------------------------------------- */
void csfg_expr_init(struct csfg_expr** expr)
{
    *expr = NULL;
}

/* ------------------------------------------------------------------------- */
void csfg_expr_deinit(struct csfg_expr* expr)
{
    if (expr != NULL)
        mem_free(expr);
}

/* ------------------------------------------------------------------------- */
int csfg_expr_parse(struct csfg_expr** expr, const char* text)
{
    struct parser p;
    parser_init(&p, text);

    if (parse_expr(&p, expr) != 0)
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

    return 0;
}

/* ------------------------------------------------------------------------- */
double csfg_expr_eval(const struct csfg_expr* expr)
{
    return 0.0;
}

/* ------------------------------------------------------------------------- */
static int new_expr(struct csfg_expr** expr, enum csfg_expr_type type)
{
    int               new_cap, n;
    struct csfg_expr* new_expr;

    if (*expr == NULL || (*expr)->count == (*expr)->capacity)
    {
        new_cap = *expr ? (*expr)->capacity * 2 : 16;
        new_expr = mem_realloc(*expr, new_cap * sizeof(struct csfg_expr));
        if (new_expr == NULL)
            return -1;
        *expr = new_expr;
    }

    n = (*expr)->count++;
  (*expr)->nodes[n].type = type;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_lit(struct csfg_expr** expr, double value)
{
    int n = new_node(expr);
    if (n == -1)
        return -1;
    return 0;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_add(struct csfg_expr** expr, int left, int right)
{
}

/* ------------------------------------------------------------------------- */
int csfg_expr_sub(struct csfg_expr** expr, int left, int right)
{
}

/* ------------------------------------------------------------------------- */
int csfg_expr_mul(struct csfg_expr** expr, int left, int right)
{
}

/* ------------------------------------------------------------------------- */
int csfg_expr_div(struct csfg_expr** expr, int left, int right)
{
}

/* ------------------------------------------------------------------------- */
int csfg_expr_pow(struct csfg_expr** expr, int base, int exp)
{
}
