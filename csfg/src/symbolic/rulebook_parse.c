#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/rulebook.h"
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
    X(BUILTIN, "builtin")                                                      \
    X(CONST, "const")                                                          \
    X(IGNORE, "ignore")                                                        \
    X(TRANSLATES_TO, "-->")

enum token
{
    TOK_ERROR = -1,
    TOK_END   = 0,
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
        int integer;
    } value;
    const char* filename;
    const char* data;
    int tail;
    int head;
    int end;
};

/* -------------------------------------------------------------------------- */
static void
parser_init(struct parser* p, const char* filename, struct strview text)
{
    p->filename = filename;
    p->data     = text.data + text.off;
    p->end      = text.len - text.off;
    p->head     = 0;
    p->tail     = 0;
}

/* -------------------------------------------------------------------------- */
static int parser_error(const struct parser* p, const char* fmt, ...)
{
    va_list ap;
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
static struct csfg_ruleset*
add_ruleset(struct csfg_ruleset_vec** rulesets, int* ruleset_idx)
{
    struct csfg_ruleset* ruleset;

    *ruleset_idx = vec_count(*rulesets);
    ruleset      = csfg_ruleset_vec_emplace(rulesets);
    if (ruleset == NULL)
        return NULL;

    ruleset->expr_search  = -1;
    ruleset->expr_replace = -1;
    ruleset->builtin_run  = NULL;
    ruleset->next         = -1;
    ruleset->child        = -1;
    ruleset->ignore       = 0;

    return ruleset;
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
            p->value.str.off  = ++p->head;
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
            p->value.str.off  = p->head++;
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
static int parse_qualifiers(struct parser* p, struct csfg_ruleset* ruleset)
{
    enum token tok;
    while (1)
    {
        tok = scan_next(p);
    reswitch_next:
        switch (tok)
        {
            case TOK_ERROR: return -1;
            case TOK_END  : return 0;

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

            case TOK_IGNORE: {
                ruleset->ignore = 1;
                tok             = scan_next(p);
                if (tok != ',')
                    goto reswitch_next;
                break;
            }

            case ']': return tok;

            default: return parser_error(p, "Unknown qualifier\n");
        }
    }
}

/* -------------------------------------------------------------------------- */
static int parse_ruleset_block(
    struct parser* p, struct csfg_rulebook* book, int* ruleset_idx);
static int
parse_block(struct parser* p, struct csfg_rulebook* book, int* ruleset_idx)
{
    enum token tok;
    int current_ruleset_idx = -1;
    *ruleset_idx            = -1;
    while (1)
    {
        tok = scan_next(p);
    reswitch_tok:
        switch (tok)
        {
            case TOK_ERROR: return -1;
            case TOK_END  : return 0;

            case '{': {
                int other_ruleset_idx;
                if (parse_ruleset_block(p, book, &other_ruleset_idx) != '}')
                    return parser_error(p, "Missing closing '}'\n");
                if (current_ruleset_idx > -1)
                    vec_get(book->rulesets, current_ruleset_idx)->next =
                        other_ruleset_idx;
                else
                    *ruleset_idx = other_ruleset_idx;
                current_ruleset_idx = other_ruleset_idx;
                break;
            }

            case TOK_STRING: {
                int next_rulest_idx;
                struct csfg_ruleset* ruleset;

                ruleset = add_ruleset(&book->rulesets, &next_rulest_idx);
                if (ruleset == NULL)
                    return -1;

                if (current_ruleset_idx > -1)
                    vec_get(book->rulesets, current_ruleset_idx)->next =
                        next_rulest_idx;
                else
                    *ruleset_idx = next_rulest_idx;
                current_ruleset_idx = next_rulest_idx;

                ruleset->expr_search =
                    csfg_expr_parse(&book->pool, p->value.str);
                if (ruleset->expr_search < 0)
                    return parser_error(
                        p, "Failed to parse search expression\n");

                if (scan_next(p) != TOK_TRANSLATES_TO)
                    return parser_error(p, "Expected --> operator\n");

                if (scan_next(p) != TOK_STRING)
                    return parser_error(p, "Expected expression\n");
                ruleset->expr_replace =
                    csfg_expr_parse(&book->pool, p->value.str);
                if (ruleset->expr_replace < 0)
                    return parser_error(
                        p, "Failed to parse replace expression\n");

                tok = scan_next(p);
                if (tok != '[')
                    goto reswitch_tok;
                tok = parse_qualifiers(p, ruleset);
                if (tok <= 0)
                    return tok;
                if (tok != ']')
                    return parser_error(p, "Missing closing ']'\n");
                break;
            }

            case TOK_IDENTIFIER: {
                int *child_ruleset_idx, next_ruleset_idx;
                struct csfg_ruleset* ruleset;

                child_ruleset_idx =
                    csfg_ruleset_hmap_find(book->ruleset_map, p->value.str);
                if (child_ruleset_idx == NULL)
                    return parser_error(p, "Ruleset name not found\n");

                ruleset = add_ruleset(&book->rulesets, &next_ruleset_idx);
                if (ruleset == NULL)
                    return -1;
                ruleset->child = *child_ruleset_idx;

                if (current_ruleset_idx > -1)
                    vec_get(book->rulesets, current_ruleset_idx)->next =
                        next_ruleset_idx;
                else
                    *ruleset_idx = next_ruleset_idx;
                current_ruleset_idx = next_ruleset_idx;

                tok = scan_next(p);
                if (tok != '[')
                    goto reswitch_tok;
                tok = parse_qualifiers(p, ruleset);
                if (tok <= 0)
                    return tok;
                if (tok != ']')
                    return parser_error(p, "Missing closing ']'\n");
                break;
            }

            default: return tok;
        }
    }
}

/* -------------------------------------------------------------------------- */
static int parse_ruleset_block(
    struct parser* p, struct csfg_rulebook* book, int* ruleset_idx)
{
    enum token tok;
    int child_ruleset_idx;
    struct csfg_ruleset* ruleset;

    ruleset = add_ruleset(&book->rulesets, ruleset_idx);
    if (ruleset == NULL)
        return -1;

    tok = parse_block(p, book, &child_ruleset_idx);
    vec_get(book->rulesets, *ruleset_idx)->child = child_ruleset_idx;

    return tok;
}

/* -------------------------------------------------------------------------- */
static int parse(struct parser* p, struct csfg_rulebook* book)
{
    enum token tok;
    while (1)
    {
        tok = scan_next(p);
        switch (tok)
        {
            case TOK_ERROR: return -1;
            case TOK_END  : return 0;

            case TOK_BUILTIN: {
                if (scan_next(p) != TOK_IDENTIFIER)
                    return parser_error(
                        p, "Expected an identifier after 'builtin'\n");
                if (strview_eq_cstr(p->value.str, "fold_constants"))
                {
                }
                else if (strview_eq_cstr(
                             p->value.str, "expand_constant_exponents"))
                {
                }
                else
                    return parser_error(p, "Unknown builtin function\n");
                break;
            }

            case TOK_IDENTIFIER: {
                struct strview ruleset_name;
                int* ruleset_idx;

                ruleset_name = p->value.str;
                switch (csfg_ruleset_hmap_emplace_or_get(
                    &book->ruleset_map, ruleset_name, &ruleset_idx))
                {
                    case HMAP_OOM: return -1;
                    case HMAP_EXISTS:
                        return parser_error(p, "Duplicate ruleset name\n");
                    case HMAP_NEW: break;
                }

                if (scan_next(p) != '{')
                    return parser_error(
                        p, "Missing opening '{' after ruleset identifier\n");
                tok = parse_ruleset_block(p, book, ruleset_idx);
                if (tok < 0)
                    return tok;
                if (tok != '}')
                    return parser_error(p, "Missing closing '}'\n");
                break;
            }

            default: return parser_error(p, "Unexpected token\n");
        }
    }
}

/* -------------------------------------------------------------------------- */
int csfg_rulebook_parse(
    struct csfg_rulebook* book, const char* filename, struct strview text)
{
    struct parser p;
    parser_init(&p, filename, text);
    if (parse(&p, book) != 0)
        return -1;
    return 0;
}
