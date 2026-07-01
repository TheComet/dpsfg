#include "csfg/io/deserialize.h"
#include "csfg/io/serialize.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/var_table.h"
#include "csfg/util/mem.h"
#include "dpsfg/plugin.h"
#include <ctype.h>
#include <gtk/gtk.h>

struct plugin_ctx
{
    const struct plugin_notify_interface* notify_interface;
    struct plugin_notify_context* notify_ctx;

    /* scratch buffer for temporary string manipulation */
    struct str* str;

    /* reference to app's substitutions table */
    struct csfg_var_table* substitutions_table;

    /* Keeps track of the original text the user typed in */

    GtkWidget* pole_zero_plot;
    GtkTextBuffer* text_buffer;
    gulong on_text_buffer_changed_handler_id;
};

enum token
{
    TOK_ERROR  = -1,
    TOK_END    = 0,
    TOK_EQUALS = '=',
    TOK_TEXT   = 256
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
    p->end  = strlen(text);
    p->head = 0;
    p->tail = 0;
}

/* -------------------------------------------------------------------------- */
static int parser_error(const struct parser* p, const char* fmt, ...)
{
    va_list ap;
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

/* -------------------------------------------------------------------------- */
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

        if (!isspace(p->data[p->head]))
        {
            p->value.str.data = p->data;
            p->value.str.off  = p->head++;
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

next_token:
    switch (scan_next(p))
    {
        case TOK_ERROR: return -1;
        case TOK_END  : break;

        case TOK_TEXT:
            var_name = p->value.str;
            if (scan_next(p) != '=')
                return parser_error(p, "Missing '='\n");
            if (scan_next(p) != TOK_TEXT)
                return parser_error(p, "Missing text after '='\n");
            if (csfg_var_table_set_parse_expr(vt, var_name, p->value.str) != 0)
                return -1;
            goto next_token;

        default: return parser_error(p, "Unexpected token\n");
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static void set_text_buffer_from_substitutions_table(struct plugin_ctx* ctx)
{
    GtkTextIter end;
    int16_t slot;
    const struct str* name;
    const struct csfg_var_table_entry* entry;

    g_signal_handler_block(
        ctx->text_buffer, ctx->on_text_buffer_changed_handler_id);
    gtk_text_buffer_set_text(ctx->text_buffer, "", -1);

    if (ctx->substitutions_table != NULL)
        hmap_for_each (ctx->substitutions_table->map, slot, name, entry)
        {
            str_clear(ctx->str);
            csfg_expr_to_str(&ctx->str, entry->pool, entry->expr);
            gtk_text_buffer_get_end_iter(ctx->text_buffer, &end);
            gtk_text_buffer_insert(ctx->text_buffer, &end, str_cstr(name), -1);
            gtk_text_buffer_get_end_iter(ctx->text_buffer, &end);
            gtk_text_buffer_insert(ctx->text_buffer, &end, " = ", -1);
            gtk_text_buffer_get_end_iter(ctx->text_buffer, &end);
            gtk_text_buffer_insert(
                ctx->text_buffer, &end, str_cstr(ctx->str), -1);
            gtk_text_buffer_get_end_iter(ctx->text_buffer, &end);
            gtk_text_buffer_insert(ctx->text_buffer, &end, "\n", -1);
        }

    g_signal_handler_unblock(
        ctx->text_buffer, ctx->on_text_buffer_changed_handler_id);
}

/* -------------------------------------------------------------------------- */
static int set_substitutions_table_from_text_buffer(struct plugin_ctx* ctx)
{
    gchar* text;
    GtkTextIter start, end;
    int rc;
    struct parser p;

    gtk_text_buffer_get_start_iter(ctx->text_buffer, &start);
    gtk_text_buffer_get_end_iter(ctx->text_buffer, &end);
    text = gtk_text_buffer_get_text(ctx->text_buffer, &start, &end, FALSE);

    parser_init(&p, text);
    csfg_var_table_clear(ctx->substitutions_table);
    rc = parse_text(&p, ctx->substitutions_table);
    g_free(text);

    return rc;
}

/* -------------------------------------------------------------------------- */
static void on_text_buffer_changed(GtkTextBuffer* buffer, gpointer user_data)
{
    struct plugin_ctx* ctx = user_data;
    (void)buffer;

    if (ctx->substitutions_table == NULL)
        return;

    if (set_substitutions_table_from_text_buffer(ctx) == 0)
        ctx->notify_interface->substitutions_changed(ctx->notify_ctx, ctx);
}
static GtkWidget* ui_pane_create(struct plugin_ctx* ctx)
{
    ctx->pole_zero_plot = gtk_text_view_new();
    ctx->text_buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(ctx->pole_zero_plot));

    ctx->on_text_buffer_changed_handler_id = g_signal_connect(
        ctx->text_buffer, "changed", G_CALLBACK(on_text_buffer_changed), ctx);

    return GTK_WIDGET(g_object_ref_sink(ctx->pole_zero_plot));
}
static void ui_pane_destroy(struct plugin_ctx* ctx, GtkWidget* ui)
{
    ctx->text_buffer = NULL;
    g_object_unref(ui);
}

/* -------------------------------------------------------------------------- */
static void substitutions_on_load(
    struct plugin_ctx* ctx, struct csfg_var_table* substitutions)
{
    ctx->substitutions_table = substitutions;
    if (gtk_text_buffer_get_char_count(ctx->text_buffer) == 0)
        set_text_buffer_from_substitutions_table(ctx);
}
static void substitutions_on_unload(struct plugin_ctx* ctx)
{
    ctx->substitutions_table = NULL;
}
static void substitutions_on_changed(struct plugin_ctx* ctx)
{
    set_text_buffer_from_substitutions_table(ctx);
}

/* -------------------------------------------------------------------------- */
static int io_on_save(struct plugin_ctx* ctx, struct serializer** ser)
{
    gchar* text;
    GtkTextIter start, end;
    int err = 0;

    serialize_lu16(ser, 0x0000); /* version */

    gtk_text_buffer_get_start_iter(ctx->text_buffer, &start);
    gtk_text_buffer_get_end_iter(ctx->text_buffer, &end);
    text = gtk_text_buffer_get_text(ctx->text_buffer, &start, &end, FALSE);
    err += serialize_cstr(ser, text);
    g_free(text);

    g_signal_handler_block(
        ctx->text_buffer, ctx->on_text_buffer_changed_handler_id);
    gtk_text_buffer_set_text(ctx->text_buffer, "", -1);
    g_signal_handler_unblock(
        ctx->text_buffer, ctx->on_text_buffer_changed_handler_id);

    return err;
}
static int io_on_load(struct plugin_ctx* ctx, struct deserializer* des)
{
    uint16_t version = deserialize_lu16(des);
    if (deserializer_err(des))
        return 0;

    switch (version)
    {
        case 0x0000: {
            const char* text = deserialize_cstr(des);
            g_signal_handler_block(
                ctx->text_buffer, ctx->on_text_buffer_changed_handler_id);
            gtk_text_buffer_set_text(ctx->text_buffer, text, -1);
            g_signal_handler_unblock(
                ctx->text_buffer, ctx->on_text_buffer_changed_handler_id);
            break;
        }
    }
    return 0;
}

/* -------------------------------------------------------------------------- */
static struct plugin_ctx* create(
    const struct plugin_notify_interface* notify_interface,
    struct plugin_notify_context* notify_ctx,
    GTypeModule* type_module)
{
    struct plugin_ctx* ctx = mem_alloc(sizeof(struct plugin_ctx));
    (void)type_module;
    ctx->notify_interface = notify_interface;
    ctx->notify_ctx       = notify_ctx;
    str_init(&ctx->str);
    ctx->substitutions_table               = NULL;
    ctx->on_text_buffer_changed_handler_id = -1;
    return ctx;
}
static void destroy(struct plugin_ctx* ctx, GTypeModule* type_module)
{
    (void)type_module;
    str_deinit(ctx->str);
    mem_free(ctx);
}

/* -------------------------------------------------------------------------- */
static struct dpsfg_ui_pane_interface ui_pane = {
    ui_pane_create, ui_pane_destroy};
static struct dpsfg_substitutions_interface substitutions = {
    substitutions_on_load, substitutions_on_unload, substitutions_on_changed};
static struct dpsfg_io_interface io = {io_on_save, io_on_load};

static struct dpsfg_plugin_info info = {
    "Substitutions",
    "editor",
    "TheComet",
    "@TheComet93",
    "Signal Flow Graph Editor"};

PLUGIN_API struct dpsfg_plugin_interface dpsfg_plugin = {
    PLUGIN_VERSION,
    0,
    &info,
    create,
    destroy,
    NULL,
    &ui_pane,
    NULL,
    &substitutions,
    NULL,
    NULL,
    NULL,
    &io};
