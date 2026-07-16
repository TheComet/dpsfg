#include "csfg/graph/graph.h"
#include "csfg/io/deserialize.h"
#include "csfg/io/io.h"
#include "csfg/io/serialize.h"
#include "graph-editor/attr.h"
#include "graph-editor/clipboard.h"
#include "graph-editor/drawing.h"
#include "graph-editor/graph_helpers.h"
#include "graph-editor/graph_model.h"
#include <gtk/gtk.h>

static const int CHUNK_SIZE = 1024;

struct read_async_ctx
{
    struct graph_model* model;
    struct serializer* ser;
    void (*paste_complete_callback)(void*);
    void* paste_complete_arg;
    int mouse_x, mouse_y;
};

/* -------------------------------------------------------------------------- */
static int id_exists(const struct csfg_graph* g, uint16_t id)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;
    csfg_graph_for_each_node (g, n)
        if (n->id == id)
            return 1;
    csfg_graph_for_each_edge (g, e)
        if (e->id == id)
            return 1;
    return 0;
}
static uint16_t
new_id(struct csfg_graph* model_graph, const struct csfg_graph* subgraph)
{
    uint16_t id = model_graph->id_counter++;
    if (model_graph->id_counter == CSFG_GRAPH_GC_ID)
        model_graph->id_counter++;
    while (id_exists(model_graph, id) || id_exists(subgraph, id))
    {
        id++;
        if (id == CSFG_GRAPH_GC_ID)
            id++;
    }
    return id;
}

/* -------------------------------------------------------------------------- */
static int lowest_graph_x(const struct csfg_graph* g)
{
    const struct csfg_node* n;
    int x = INT_MAX;

    csfg_graph_for_each_node (g, n)
        if (x > n->x)
            x = n->x;

    return x;
}
static int lowest_graph_y(const struct csfg_graph* g)
{
    const struct csfg_node* n;
    int y = INT_MAX;

    csfg_graph_for_each_node (g, n)
        if (y > n->y)
            y = n->y;

    return y == INT_MAX ? 0 : y;
}

/* -------------------------------------------------------------------------- */
static int merge_and_select_subgraph(
    struct deserializer* des,
    struct csfg_graph* model_graph,
    struct csfg_graph* subgraph,
    struct node_attr_hmap** node_attrs,
    struct edge_attr_hmap** edge_attrs,
    struct line_vec** drawing,
    int mouse_x,
    int mouse_y)
{
    struct csfg_node* n;
    struct csfg_edge* e;
    struct node_attr* na;
    struct edge_attr* ea;
    int idx_offset, total_nodes, total_edges, count, id, offset_x, offset_y;

    idx_offset  = csfg_graph_node_count(model_graph);
    total_nodes = vec_count(model_graph->nodes) + vec_count(subgraph->nodes);
    total_edges = vec_count(model_graph->edges) + vec_count(subgraph->edges);

    offset_x = lowest_graph_x(subgraph);
    offset_y = lowest_graph_y(subgraph);

    count = deserialize_lu16(des);
    while (count--)
    {
        id = deserialize_lu16(des);
        n  = find_node(subgraph, id);
        if (n == NULL)
            return -1;

        if (id_exists(model_graph, id))
            n->id = new_id(model_graph, subgraph);

        n->x += mouse_x - offset_x;
        n->y += mouse_y - offset_y;

        switch (node_attr_hmap_emplace_or_get(node_attrs, n->id, &na))
        {
            case HMAP_OOM   : return -1;
            case HMAP_EXISTS: return -1;
            case HMAP_NEW   : node_attr_init(na, rgb(0, 0, 0));
        }
        if (node_attr_load(des, na) != 0)
            return -1;
        na->selected = 1;
    }

    count = deserialize_lu16(des);
    while (count--)
    {
        id = deserialize_lu16(des);
        e  = find_edge(subgraph, id);
        if (e == NULL)
            return -1;

        if (id_exists(model_graph, id))
            e->id = new_id(model_graph, subgraph);

        e->x += mouse_x - offset_x;
        e->y += mouse_y - offset_y;

        switch (edge_attr_hmap_emplace_or_get(edge_attrs, e->id, &ea))
        {
            case HMAP_OOM   : return -1;
            case HMAP_EXISTS: return -1;
            case HMAP_NEW   : edge_attr_init(ea, rgb(0, 0, 0));
        }
        if (edge_attr_load(des, ea) != 0)
            return -1;
    }

    if (vec_capacity(model_graph->nodes) < total_nodes)
        if (csfg_node_vec_realloc(&model_graph->nodes, total_nodes) != 0)
            return -1;
    if (vec_capacity(model_graph->edges) < total_edges)
        if (csfg_edge_vec_realloc(&model_graph->edges, total_edges) != 0)
            return -1;
    csfg_graph_for_each_edge (subgraph, e)
    {
        e->n_idx_from += idx_offset;
        e->n_idx_to += idx_offset;
    }
    csfg_graph_for_each_node (subgraph, n)
        csfg_node_vec_push_no_realloc(model_graph->nodes, *n);
    csfg_graph_for_each_edge (subgraph, e)
        csfg_edge_vec_push_no_realloc(model_graph->edges, *e);

    count = deserialize_lu16(des);
    while (count--)
    {
        struct point* point;
        struct line* line = line_vec_emplace(drawing);
        if (line == NULL)
            return -1;

        line_init(line, rgb(0, 0, 0));
        if (line_load(des, line) != 0)
            return -1;

        if (offset_x == INT_MAX)
        {
            if (vec_count(line->points) > 0)
            {
                offset_x = vec_first(line->points)->x;
                offset_y = vec_first(line->points)->y;
            }
            else
            {
                offset_x = 0;
                offset_y = 0;
            }
        }

        vec_for_each (line->points, point)
        {
            point->x += mouse_x - offset_x;
            point->y += mouse_y - offset_y;
        }

        line->selected = 1;
    }

    csfg_node_vec_clear(subgraph->nodes);
    csfg_edge_vec_clear(subgraph->edges);

    return deserializer_err(des);
}

/* -------------------------------------------------------------------------- */
static int insert_graph_data_into_model(
    struct deserializer* des,
    struct graph_model* model,
    int mouse_x,
    int mouse_y)
{
    int node_in, node_out;
    struct csfg_graph subgraph;

    if (model->graph == NULL)
        return 0;

    csfg_graph_init(&subgraph);
    if (csfg_io_graph_load(des, &subgraph, &node_in, &node_out) != 0)
        goto load_subgraph_failed;

    multi_deselect_all(model);

    if (merge_and_select_subgraph(
            des,
            model->graph,
            &subgraph,
            &model->node_attrs,
            &model->edge_attrs,
            &model->drawing,
            mouse_x,
            mouse_y) != 0)
        goto load_subgraph_failed;

    csfg_graph_deinit(&subgraph);
    return 0;

load_subgraph_failed:
    csfg_graph_deinit(&subgraph);
    return -1;
}

/* -------------------------------------------------------------------------- */
static void read_async_graph_data_cb(
    GObject* source_object, GAsyncResult* res, gpointer data)
{
    struct deserializer des;
    struct read_async_ctx* ctx = data;
    GError* error              = NULL;
    GInputStream* stream       = G_INPUT_STREAM(source_object);
    int count = g_input_stream_read_finish(stream, res, &error);
    if (count < 0 || error)
    {
        log_err("read failed: %s\n", error->message);
        g_error_free(error);
        serializer_deinit(ctx->ser);
        mem_free(ctx);
        return;
    }

    if (count > 0 || count == CHUNK_SIZE)
    {
        ctx->ser->count += count;
        g_input_stream_read_async(
            stream,
            vec_data(ctx->ser) + vec_count(ctx->ser),
            6,
            G_PRIORITY_DEFAULT,
            NULL,
            read_async_graph_data_cb,
            ctx);
        return;
    }

    des = deserializer(vec_data(ctx->ser), vec_count(ctx->ser));
    log_dbg("read %d bytes\n", vec_count(ctx->ser));
    insert_graph_data_into_model(&des, ctx->model, ctx->mouse_x, ctx->mouse_y);
    serializer_deinit(ctx->ser);

    ctx->paste_complete_callback(ctx->paste_complete_arg);
    mem_free(ctx);
}

/* -------------------------------------------------------------------------- */
static void clipboard_graph_data_cb(
    GObject* source_object, GAsyncResult* res, gpointer user_data)
{
    struct read_async_ctx* ctx = user_data;
    GError* error              = NULL;
    GInputStream* stream       = gdk_clipboard_read_finish(
        GDK_CLIPBOARD(source_object), res, NULL, &error);

    if (stream == NULL)
    {
        log_err("read failed: %s\n", error->message);
        g_error_free(error);
        goto read_finish_failed;
    }

    if (serializer_realloc(&ctx->ser, CHUNK_SIZE) != 0)
        goto realloc_serializer_failed;

    g_input_stream_read_async(
        stream,
        vec_data(ctx->ser),
        CHUNK_SIZE,
        G_PRIORITY_DEFAULT,
        NULL,
        read_async_graph_data_cb,
        ctx);
    g_object_unref(stream);
    return;

realloc_serializer_failed:
    g_object_unref(stream);
read_finish_failed:;
    mem_free(ctx);
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
void paste_from_clipboard(
    GdkClipboard* clipboard,
    struct graph_model* model,
    int mouse_x,
    int mouse_y,
    void (*paste_complete_callback)(void*),
    void* user_data)
{
    GdkContentFormats* formats = gdk_clipboard_get_formats(clipboard);
    if (gdk_content_formats_contain_mime_type(
            formats, "application/x-dpsfg-graph-editor"))
    {
        const char* mime_types[]   = {"application/x-dpsfg-graph-editor", NULL};
        struct read_async_ctx* ctx = mem_alloc(sizeof *ctx);
        if (ctx == NULL)
            goto alloc_ctx_failed;
        serializer_init(&ctx->ser);
        ctx->model                   = model;
        ctx->paste_complete_callback = paste_complete_callback;
        ctx->paste_complete_arg      = user_data;
        ctx->mouse_x                 = mouse_x;
        ctx->mouse_y                 = mouse_y;
        gdk_clipboard_read_async(
            clipboard,
            mime_types,
            G_PRIORITY_DEFAULT,
            NULL,
            clipboard_graph_data_cb,
            ctx);
    alloc_ctx_failed:;
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
node_is_selected(const struct graph_model* model, const struct csfg_node* n)
{
    return node_attr_hmap_find(model->node_attrs, n->id)->selected;
}
static int edge_is_connected_to_selected_nodes(
    const struct csfg_edge* e, const struct graph_model* model)
{
    const struct csfg_node* n1 =
        csfg_graph_get_node(model->graph, e->n_idx_from);
    const struct csfg_node* n2 = csfg_graph_get_node(model->graph, e->n_idx_to);
    return node_is_selected(model, n1) && node_is_selected(model, n2);
}
void copy_selection_to_clipboard(
    GdkClipboard* clipboard, const struct graph_model* model)
{
    GBytes* bytes;
    GdkContentProvider* provider;
    struct serializer* ser;
    struct csfg_graph subgraph;
    const struct csfg_node *n, *n1, *n2;
    const struct csfg_edge* e;
    const struct node_attr* na;
    const struct edge_attr* ea;
    const struct line* line;
    int count;
    int err = 0;

    if (model->graph == NULL)
        return;

    serializer_init(&ser);
    csfg_graph_init(&subgraph);

    csfg_graph_for_each_node (model->graph, n)
        if (node_is_selected(model, n))
            if (csfg_node_vec_push(&subgraph.nodes, *n) != 0)
                goto serialize_failed;

    csfg_graph_for_each_edge (model->graph, e)
        if (edge_is_connected_to_selected_nodes(e, model))
        {
            struct csfg_edge* e_new = csfg_edge_vec_emplace(&subgraph.edges);
            if (e_new == NULL)
                goto serialize_failed;
            *e_new = *e;

            /* Fix up indices, as we are probably not copying the whole graph */
            n1 = csfg_graph_get_node(model->graph, e->n_idx_from);
            n2 = csfg_graph_get_node(model->graph, e->n_idx_to);
            e_new->n_idx_from = find_node_idx(&subgraph, n1->id);
            e_new->n_idx_to   = find_node_idx(&subgraph, n2->id);
        }

    err += csfg_io_graph_save(&ser, &subgraph, -1, -1);

    err += serialize_lu16(&ser, vec_count(subgraph.nodes));
    vec_for_each (subgraph.nodes, n)
    {
        na = node_attr_hmap_find(model->node_attrs, n->id);
        err += serialize_lu16(&ser, n->id);
        err += node_attr_save(&ser, na);
    }
    err += serialize_lu16(&ser, vec_count(subgraph.edges));
    vec_for_each (subgraph.edges, e)
    {
        ea = edge_attr_hmap_find(model->edge_attrs, e->id);
        err += serialize_lu16(&ser, e->id);
        err += edge_attr_save(&ser, ea);
    }

    count = 0;
    vec_for_each (model->drawing, line)
        if (line->selected)
            count++;
    err += serialize_lu16(&ser, count);
    vec_for_each (model->drawing, line)
        if (line->selected)
            err += line_save(&ser, line);

    bytes    = g_bytes_new(vec_data(ser), vec_count(ser));
    provider = gdk_content_provider_new_for_bytes(
        "application/x-dpsfg-graph-editor", bytes);
    gdk_clipboard_set_content(clipboard, provider);
    g_object_unref(provider);
    log_dbg("Wrote %d bytes\n", vec_count(ser));

serialize_failed:
    serializer_deinit(ser);
    /* subgraph holds shallow copies of nodes and edges, so we must explicitly
     * deinit just the node and edge arrays */
    csfg_node_vec_deinit(subgraph.nodes);
    csfg_edge_vec_deinit(subgraph.edges);
    (void)err;
}
