#include "csfg/symbolic/var_table.h"
#include "csfg/util/mem.h"
#include "dpsfg/plugin.h"
#include <ctype.h>
#include <gtk/gtk.h>

struct plugin_ctx
{
    GtkWidget*                               subtitutions_view;
    const struct plugin_callbacks_interface* icb;
    struct plugin_callbacks*                 cb;
    struct csfg_var_table*                   substitutions_table;
};

enum token
{
    TOK_ERROR = -1,
    TOK_END = 0,
    TOK_EQUALS = '=',
    TOK_TEXT = 256
};

struct parser
{
    const char* data;
    union
    {
        struct strview str;
    } value;
    int head, tail, end;
};

/* -------------------------------------------------------------------------- */
static void parser_init(struct parser* p, const char* text)
{
    p->data = text;
    p->end = strlen(text);
    p->head = 0;
    p->tail = 0;
}

/* ------------------------------------------------------------------------- */
static int parser_error(const struct parser* p, const char* fmt, ...)
{
    va_list        ap;
    struct strview loc;

    loc.off = p->tail;
    loc.len = p->head - p->tail;

    fprintf(
        stderr,
        "[substitutions] parser error %d-%d: ",
        loc.off,
        loc.off + loc.len);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    return -1;
}

/* ------------------------------------------------------------------------- */
static enum token scan_next(struct parser* p)
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
        if (p->data[p->head] == '#')
        {
            /* Find end of comment */
            for (p->head += 1; p->head != p->end; p->head++)
                if (p->data[p->head] == '\n')
                {
                    p->head++;
                    break;
                }
            continue;
        }

        if (p->data[p->head] == '=')
        {
            p->head++;
            return '=';
        }

        if (isalnum(p->data[p->head]) || p->data[p->head] == '_')
        {
            p->value.str.data = p->data;
            p->value.str.off = p->head++;
            while (p->head != p->end &&
                   (p->data[p->head] != '\n' && p->data[p->head] != '='))
            {
                p->head++;
            }
            p->value.str.len = p->head - p->value.str.off;
            while (p->data[p->value.str.off + p->value.str.len - 1] == ' ')
                p->value.str.len--;
            return TOK_TEXT;
        }

        p->tail = ++p->head;
    }

    return TOK_END;
}

/* -------------------------------------------------------------------------- */
static int parse_text(struct parser* p, struct csfg_var_table* vt)
{
    struct strview var_name;
    int            modified = 0;

next_token:
    switch (scan_next(p))
    {
        case TOK_ERROR: return -1;
        case TOK_END: break;

        case TOK_TEXT:
            var_name = p->value.str;
            if (scan_next(p) != '=')
                return parser_error(p, "Missing '='\n");
            if (scan_next(p) != TOK_TEXT)
                return parser_error(p, "Missing text after '='\n");
            if (csfg_var_table_set_parse_expr(vt, var_name, p->value.str) != 0)
                return -1;
            modified = 1;
            goto next_token;

        default: return parser_error(p, "Unexpected token\n");
    }

    return modified;
}

/* -------------------------------------------------------------------------- */
static void on_text_buffer_changed(GtkTextBuffer* buffer, gpointer user_data)
{
    gchar*             text;
    GtkTextIter        start, end;
    int                modified;
    struct parser      p;
    struct plugin_ctx* ctx = user_data;

    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    parser_init(&p, text);
    csfg_var_table_clear(ctx->substitutions_table);
    modified = parse_text(&p, ctx->substitutions_table);

    g_free(text);

    if (modified)
        ctx->icb->substitutions_changed(ctx->cb, ctx);
}
static GtkWidget* ui_pane_create(struct plugin_ctx* ctx)
{
    ctx->subtitutions_view = gtk_text_view_new();
    GtkTextBuffer* buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(ctx->subtitutions_view));

    g_signal_connect(
        buffer, "changed", G_CALLBACK(on_text_buffer_changed), ctx);

    return GTK_WIDGET(g_object_ref_sink(ctx->subtitutions_view));
}
static void ui_pane_destroy(struct plugin_ctx* ctx, GtkWidget* ui)
{
    (void)ctx;
    g_object_unref(ui);
}

/* -------------------------------------------------------------------------- */
void substitutions_on_set(
    struct plugin_ctx* ctx, struct csfg_var_table* substitutions)
{
    ctx->substitutions_table = substitutions;
}
void substitutions_on_changed(
    struct plugin_ctx* ctx, struct csfg_var_table* substitutions)
{
    (void)ctx, (void)substitutions;
}
void substitutions_on_clear(struct plugin_ctx* ctx)
{
    (void)ctx;
}

/* -------------------------------------------------------------------------- */
static struct plugin_ctx* create(
    const struct plugin_callbacks_interface* icb,
    struct plugin_callbacks*                 cb,
    GTypeModule*                             type_module)
{
    struct plugin_ctx* ctx = mem_alloc(sizeof(struct plugin_ctx));
    (void)type_module;
    ctx->icb = icb;
    ctx->cb = cb;
    return ctx;
}
static void destroy(struct plugin_ctx* ctx, GTypeModule* type_module)
{
    (void)type_module;
    mem_free(ctx);
}

/* -------------------------------------------------------------------------- */
static struct dpsfg_ui_pane_interface ui_pane = {
    ui_pane_create, ui_pane_destroy};
static struct dpsfg_substitutions_interface substitutions = {
    &substitutions_on_set, &substitutions_on_changed, &substitutions_on_clear};

static struct plugin_info info = {
    "Substitutions",
    "editor",
    "TheComet",
    "@TheComet93",
    "Signal Flow Graph Editor"};

PLUGIN_API struct plugin_interface dpsfg_plugin = {
    PLUGIN_VERSION,
    0,
    &info,
    create,
    destroy,
    NULL,
    &ui_pane,
    NULL,
    &substitutions,
    NULL};
