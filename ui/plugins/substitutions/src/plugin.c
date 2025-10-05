#include "csfg/symbolic/var_table.h"
#include "csfg/util/mem.h"
#include "dpsfg/plugin.h"
#include <gtk/gtk.h>

struct plugin_ctx
{
    GtkWidget*                               subtitutions_view;
    const struct plugin_callbacks_interface* icb;
    struct plugin_callbacks*                 cb;
    struct csfg_var_table*                   substitutions_table;
};

/* -------------------------------------------------------------------------- */
static void on_text_buffer_changed(GtkTextBuffer* buffer, gpointer user_data)
{
    gchar*             text;
    GtkTextIter        start, end;
    struct strview     var_name, expr;
    int                off, state, modified;
    struct plugin_ctx* ctx = user_data;

    csfg_var_table_clear(ctx->substitutions_table);

    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    var_name.data = text;
    var_name.off = 0;
    var_name.len = 0;
    expr.data = text;
    state = 0;
    modified = 0;

    for (off = 0;; ++off)
    {
        switch (state)
        {
            case 0:
                if (text[off] == '=' && text[off] != '\0')
                {
                    expr.off = off + 1;
                    expr.len = 0;
                    state = 1;
                }
                else
                    var_name.len++;
                break;

            case 1:
                if (text[off] == '\n' || text[off] == '\0')
                {
                    if (var_name.len > 0 && expr.len > 0 &&
                        csfg_var_table_set_parse_expr(
                            ctx->substitutions_table, var_name, expr) == 0)
                    {
                        modified = 1;
                    }

                    var_name.off = expr.off + expr.len + 1;
                    var_name.len = 0;
                    state = 0;
                }
                else
                    expr.len++;
                break;
        }

        if (text[off] == '\0')
            break;
    }

    g_free(text);

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
}
void substitutions_on_clear(struct plugin_ctx* ctx)
{
}

/* -------------------------------------------------------------------------- */
static struct plugin_ctx* create(
    const struct plugin_callbacks_interface* icb,
    struct plugin_callbacks*                 cb,
    GTypeModule*                             type_module)
{
    struct plugin_ctx* ctx = mem_alloc(sizeof(struct plugin_ctx));
    ctx->icb = icb;
    ctx->cb = cb;
    return ctx;
}
static void destroy(struct plugin_ctx* ctx, GTypeModule* type_module)
{
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
