#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include "csfg/util/log.h"
#include "csfg/util/strview.h"
#include <ctype.h>

#define CHAR_TOKEN_LIST                                                        \
    X(LBRACE, '{')                                                             \
    X(RBRACE, '}')                                                             \
    X(LBRACKET, '[')                                                           \
    X(RBRACKET, ']')                                                           \
    X(EQUALS, '=')                                                             \
    X(COMMA, ',')

#define KEYWORD_LIST                                                           \
    X(EXTERN, "extern")                                                        \
    X(CONST, "const")                                                          \
    X(TRANSLATES_TO, "-->")

enum token
{
    TOK_ERROR = -1,
    TOK_END = 0,
#define X(name, char) TOK_##name = char,
    CHAR_TOKEN_LIST
#undef X
        TOK_IDENTIFIER = 256,
#define X(name, str) TOK_##name,
    KEYWORD_LIST
#undef X
        TOK_STRING
};

struct parser
{
    union
    {
        struct strview str;
        int            integer;
    } value;
    const char* filename;
    const char* data;
    int         tail;
    int         head;
    int         end;
};

/* -------------------------------------------------------------------------- */
static void
parser_init(struct parser* p, const char* filename, struct strview text)
{
    p->filename = filename;
    p->data = text.data + text.off;
    p->end = text.len - text.off;
    p->head = 0;
    p->tail = 0;
}

/* -------------------------------------------------------------------------- */
static int parser_error(const struct parser* p, const char* fmt, ...)
{
    va_list        ap;
    struct strspan loc;

    loc.off = p->tail;
    loc.len = p->head - p->tail;

    va_start(ap, fmt);
    log_vflc(p->filename, p->data, loc, fmt, ap);
    va_end(ap);
    log_excerpt(p->data, loc);

    return -1;
}

/* -------------------------------------------------------------------------- */
enum token scan_next(struct parser* p)
{
    p->tail = p->head;
    while (p->head != p->end)
    {
        if (p->data[p->head] == '/' && p->data[p->head + 1] == '*')
        {
            /* Find end of comment */
            for (p->head += 2; p->head != p->end; p->head++)
                if (p->data[p->head] == '*' && p->data[p->head + 1] == '/')
                {
                    p->head += 2;
                    break;
                }
            if (p->head == p->end)
                return parser_error(p, "Missing closing comment\n");
            continue;
        }
        if (p->data[p->head] == '/' && p->data[p->head + 1] == '/')
        {
            /* Find end of comment */
            for (p->head += 2; p->head != p->end; p->head++)
                if (p->data[p->head] == '\n')
                {
                    p->head++;
                    break;
                }
            continue;
        }

        /* String literal ".*?" (spans over newlines)*/
        if (p->data[p->head] == '"')
        {
            p->value.str.data = p->data;
            p->value.str.off = ++p->head;
            for (; p->head != p->end; ++p->head)
            {
                if (p->data[p->head] == '"' && p->data[p->head - 1] != '\\')
                    break;
                if (p->data[p->head] == '\n')
                    return parser_error(p, "Missing closing quote on string\n");
            }
            if (p->head == p->end)
                return parser_error(p, "Missing closing quote on string\n");
            p->value.str.len = p->head++ - p->value.str.off;
            return TOK_STRING;
        }

        /* Single characters */
#define X(name, char)                                                          \
    if (p->data[p->head] == char)                                              \
        return p->data[p->head++];
        CHAR_TOKEN_LIST
#undef X

        /* Keywords */
#define X(tok, name)                                                           \
    if (p->end - p->head >= (int)sizeof(name) - 1 &&                           \
        memcmp(p->data + p->head, name, sizeof(name) - 1) == 0)                \
    {                                                                          \
        p->head += sizeof(name) - 1;                                           \
        return TOK_##tok;                                                      \
    }
        KEYWORD_LIST
#undef X

        /* Identifier [a-zA-Z_-][a-zA-Z0-9_-]* */
        if (isalpha(p->data[p->head]) || p->data[p->head] == '_')
        {
            p->value.str.data = p->data;
            p->value.str.off = p->head++;
            while (p->head != p->end &&
                   (isalnum(p->data[p->head]) || p->data[p->head] == '_'))
            {
                p->head++;
            }
            p->value.str.len = p->head - p->value.str.off;
            return TOK_IDENTIFIER;
        }

        p->tail = ++p->head;
    }

    return TOK_END;
}

/* -------------------------------------------------------------------------- */
static int parse_qualifiers(struct parser* p)
{
    enum token tok;
    while (1)
    {
        tok = scan_next(p);
    reswitch_next:
        switch (tok)
        {
            case TOK_ERROR: return -1;
            case TOK_END: return 0;

            case TOK_IDENTIFIER: {
                if (scan_next(p) != '=')
                    return parser_error(p, "Expected '='\n");
                tok = scan_next(p);
                if (tok == TOK_CONST)
                {
                }
                else
                    return parser_error(p, "Unexpected token\n");

                tok = scan_next(p);
                if (tok != ',')
                    goto reswitch_next;
                break;
            }

            default: return tok;
        }
    }
}

/* -------------------------------------------------------------------------- */
static int
parse_pseudo_block(struct parser* p, struct csfg_expr_op* op, int* pseudo_idx);
static int
parse_block(struct parser* p, struct csfg_expr_op* op, int* group_idx)
{
    enum token tok;
    int        current_group_idx = -1;
    *group_idx = -1;
    while (1)
    {
        tok = scan_next(p);
    reswitch_tok:
        switch (tok)
        {
            case TOK_ERROR: return -1;
            case TOK_END: return 0;

            case '{': {
                int other_pseudo_idx;
                if (parse_pseudo_block(p, op, &other_pseudo_idx) != '}')
                    return parser_error(
                        p, "Missing closing '}' of operation block\n");
                if (current_group_idx > -1)
                    vec_get(op->groups, current_group_idx)->next =
                        other_pseudo_idx;
                else
                    *group_idx = other_pseudo_idx;
                current_group_idx = other_pseudo_idx;
                break;
            }

            case TOK_STRING: {
                int next_group_idx = vec_count(op->groups);
                struct csfg_expr_op_group* group =
                    csfg_expr_op_group_vec_emplace(&op->groups);
                if (group == NULL)
                    return -1;
                group->expr_from = -1;
                group->expr_to = -1;
                group->extern_pass = NULL;
                group->next = -1;
                group->child = -1;
                if (current_group_idx > -1)
                    vec_get(op->groups, current_group_idx)->next =
                        next_group_idx;
                else
                    *group_idx = next_group_idx;
                current_group_idx = next_group_idx;

                group->expr_from = csfg_expr_parse(&op->pool, p->value.str);
                if (group->expr_from < 0)
                    return parser_error(p, "Failed to parse expression\n");

                if (scan_next(p) != TOK_TRANSLATES_TO)
                    return parser_error(p, "Expected --> operator\n");

                if (scan_next(p) != TOK_STRING)
                    return parser_error(p, "Expected expression\n");
                group->expr_to = csfg_expr_parse(&op->pool, p->value.str);
                if (group->expr_to < 0)
                    return parser_error(p, "Failed to parse expression\n");

                tok = scan_next(p);
                if (tok != '[')
                    goto reswitch_tok;
                if (parse_qualifiers(p) != ']')
                    return parser_error(p, "Missing closing ']'\n");
                break;
            }

            case TOK_IDENTIFIER: {
                int *                      pseudo_group_idx, next_group_idx;
                struct csfg_expr_op_group* group;

                pseudo_group_idx =
                    csfg_expr_op_hmap_find(op->group_map, p->value.str);
                if (pseudo_group_idx == NULL)
                    return parser_error(p, "Group name not found\n");

                next_group_idx = vec_count(op->groups);
                group = csfg_expr_op_group_vec_emplace(&op->groups);
                if (group == NULL)
                    return -1;
                group->expr_from = -1;
                group->expr_to = -1;
                group->extern_pass = NULL;
                group->next = -1;
                group->child = *pseudo_group_idx;
                if (current_group_idx > -1)
                    vec_get(op->groups, current_group_idx)->next =
                        next_group_idx;
                else
                    *group_idx = next_group_idx;
                current_group_idx = next_group_idx;

                break;
            }

            default: return tok;
        }
    }
}

/* -------------------------------------------------------------------------- */
static int
parse_pseudo_block(struct parser* p, struct csfg_expr_op* op, int* pseudo_idx)
{
    enum token                 tok;
    int                        group_idx;
    struct csfg_expr_op_group* group;
    *pseudo_idx = vec_count(op->groups);
    group = csfg_expr_op_group_vec_emplace(&op->groups);
    if (group == NULL)
        return -1;
    group->expr_from = -1;
    group->expr_to = -1;
    group->extern_pass = NULL;
    group->next = -1;
    group->child = -1;

    tok = parse_block(p, op, &group_idx);
    vec_get(op->groups, *pseudo_idx)->child = group_idx;

    return tok;
}

/* -------------------------------------------------------------------------- */
static int parse(struct parser* p, struct csfg_expr_op* op)
{
    enum token tok;
    while (1)
    {
        tok = scan_next(p);
        switch (tok)
        {
            case TOK_ERROR: return -1;
            case TOK_END: return 0;

            case TOK_EXTERN: {
                if (scan_next(p) != TOK_IDENTIFIER)
                    return parser_error(
                        p, "Expected an identifier after 'extern'\n");
                if (strview_eq_cstr(p->value.str, "fold_constants"))
                {
                }
                else if (strview_eq_cstr(
                             p->value.str, "expand_constant_exponents"))
                {
                }
                else
                    return parser_error(p, "Unknown external function\n");
                break;
            }

            case TOK_IDENTIFIER: {
                struct strview group_name;
                int*           group_idx;

                group_name = p->value.str;
                switch (csfg_expr_op_hmap_emplace_or_get(
                    &op->group_map, group_name, &group_idx))
                {
                    case HMAP_OOM: return -1;
                    case HMAP_EXISTS:
                        return parser_error(p, "Duplicate group name\n");
                    case HMAP_NEW: break;
                }

                if (scan_next(p) != '{')
                    return parser_error(
                        p, "Missing opening '{' after operation identifier\n");
                tok = parse_pseudo_block(p, op, group_idx);
                if (tok != '}')
                    return parser_error(
                        p, "Missing closing '}' of operation block\n");
                break;
            }

            default: return parser_error(p, "Unexpected token\n");
        }
    }
}

/* -------------------------------------------------------------------------- */
int csfg_expr_op_parse_def(
    struct csfg_expr_op* op, const char* filename, struct strview text)
{
    struct parser p;
    parser_init(&p, filename, text);
    if (parse(&p, op) != 0)
        return -1;
    return 0;
}
