#include "csfg/graph/graph.h"
#include "csfg/io/deserialize.h"
#include "csfg/io/serialize.h"
#include "csfg/util/str.h"
#include "graph-editor/attr.h"
#include "graph-editor/clipboard.h"
#include "graph-editor/drawing.h"
#include "graph-editor/graph_model.h"
#include <gtk/gtk.h>

/* -------------------------------------------------------------------------- */
static void clipboard_graph_data_cb(
    GObject* source_object, GAsyncResult* res, gpointer user_data)
{
    char buf[2];
    int count;
    struct deserializer des;
    GError* error        = NULL;
    GInputStream* stream = gdk_clipboard_read_finish(
        GDK_CLIPBOARD(source_object), res, NULL, &error);
    if (stream == NULL)
    {
        log_err("read failed: %s\n", error->message);
        g_error_free(error);
        return;
    }
    if (g_input_stream_read(stream, buf, 2, NULL, &error) == 2)
    {
        des   = deserializer(buf, 2);
        count = deserialize_lu16(&des);
        log_dbg("there are %d nodes\n", count);
    }
    g_object_unref(stream);
}

/* -------------------------------------------------------------------------- */
static void
clipboard_texture_cb(GObject* source, GAsyncResult* result, gpointer user_data)
{
    GError* error = NULL;
    const GValue* value =
        gdk_clipboard_read_value_finish(GDK_CLIPBOARD(source), result, &error);
    if (value == NULL)
    {
        log_err("%s", error->message);
        g_clear_error(&error);
        return;
    }
    GdkTexture* texture = g_value_get_object(value);
    if (texture)
    {
        g_print("Got texture\n");
    }
}

/* -------------------------------------------------------------------------- */
void paste_from_clipboard(GdkClipboard* clipboard, struct graph_model* model)
{
    GdkContentFormats* formats = gdk_clipboard_get_formats(clipboard);
    if (gdk_content_formats_contain_mime_type(
            formats, "application/x-dpsfg-graph-editor"))
    {
        const char* mime_types[] = {"application/x-dpsfg-graph-editor", NULL};
        gdk_clipboard_read_async(
            clipboard,
            mime_types,
            G_PRIORITY_DEFAULT,
            NULL,
            clipboard_graph_data_cb,
            model);
    }
    else
    {
        gdk_clipboard_read_value_async(
            clipboard,
            GDK_TYPE_TEXTURE,
            G_PRIORITY_DEFAULT,
            NULL,
            clipboard_texture_cb,
            model);
    }
}

/* -------------------------------------------------------------------------- */
static int
node_is_selected(const struct csfg_node* n, const struct graph_model* model)
{
    const struct node_attr* na = node_attr_hmap_find(model->node_attrs, n->id);
    return n->id == model->active_node_id || na->selected;
}
static int
edge_is_selected(const struct csfg_edge* e, const struct graph_model* model)
{
    const struct csfg_node* n1 =
        csfg_graph_get_node(model->graph, e->n_idx_from);
    const struct csfg_node* n2 = csfg_graph_get_node(model->graph, e->n_idx_to);
    const struct node_attr* na1 =
        node_attr_hmap_find(model->node_attrs, n1->id);
    const struct node_attr* na2 =
        node_attr_hmap_find(model->node_attrs, n2->id);
    return e->id == model->active_edge_id ||
           ((n1->id == model->active_node_id || na1->selected) &&
            (n2->id == model->active_node_id || na2->selected));
}
void copy_selection_to_clipboard(
    GdkClipboard* clipboard, const struct graph_model* model)
{
    GBytes* bytes;
    GdkContentProvider* provider;
    struct serializer* ser;
    const struct csfg_node* n;
    const struct csfg_edge* e;
    const struct node_attr* na;
    const struct edge_attr* ea;
    const struct line* line;
    int count;
    int err = 0;

    if (model->graph == NULL)
        return;

    serializer_init(&ser);

    count = 0;
    csfg_graph_for_each_node (model->graph, n)
        if (node_is_selected(n, model))
            count++;
    err += serialize_lu16(&ser, count);
    csfg_graph_for_each_node (model->graph, n)
        if (node_is_selected(n, model))
        {
            na = node_attr_hmap_find(model->node_attrs, n->id);
            err += serialize_lu16(&ser, n->id);
            err += serialize_cstr(&ser, str_cstr(n->name));
            err += serialize_li16(&ser, n->x);
            err += serialize_li16(&ser, n->y);
            err += serialize_u8(&ser, na->radius);
            err += serialize_u8(&ser, na->color.r);
            err += serialize_u8(&ser, na->color.g);
            err += serialize_u8(&ser, na->color.b);
        }

    count = 0;
    csfg_graph_for_each_edge (model->graph, e)
        if (edge_is_selected(e, model))
            count++;
    err += serialize_lu16(&ser, count);
    csfg_graph_for_each_edge (model->graph, e)
        if (edge_is_selected(e, model))
        {
            ea = edge_attr_hmap_find(model->edge_attrs, e->id);
            err += serialize_lu16(&ser, e->id);
            err += serialize_cstr(&ser, str_cstr(ea->expr_str));
            err += serialize_u8(&ser, ea->color.r);
            err += serialize_u8(&ser, ea->color.g);
            err += serialize_u8(&ser, ea->color.b);
        }

    count = 0;
    vec_for_each (model->drawing, line)
        if (line->selected)
            count++;
    err += serialize_lu16(&ser, count);
    vec_for_each (model->drawing, line)
        if (line->selected)
            err += drawing_save_line(&ser, line);

    bytes    = g_bytes_new(vec_data(ser), vec_count(ser));
    provider = gdk_content_provider_new_for_bytes(
        "application/x-dpsfg-graph-editor", bytes);
    gdk_clipboard_set_content(clipboard, provider);
    g_object_unref(provider);

    g_print("Wrote %d bytes to clipboard\n", vec_count(ser));
    serializer_deinit(ser);
    (void)err;
}
