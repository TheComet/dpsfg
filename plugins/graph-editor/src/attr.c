#include "csfg/graph/graph.h"
#include "csfg/io/deserialize.h"
#include "csfg/io/serialize.h"
#include "csfg/util/str.h"
#include "graph-editor/attr.h"
#include "graph-editor/constants.h"

HMAP_DEFINE(extern, node_attr_hmap, int, struct node_attr, 16)
HMAP_DEFINE(extern, edge_attr_hmap, int, struct edge_attr, 16)

/* -------------------------------------------------------------------------- */
void edge_attr_init(struct edge_attr* ea, struct color color)
{
    str_init(&ea->expr_str);
    ea->color = color;

    ea->drag_begin_x = 0;
    ea->drag_begin_y = 0;
}

/* -------------------------------------------------------------------------- */
void edge_attr_deinit(struct edge_attr* ea)
{
    str_deinit(ea->expr_str);
}

/* -------------------------------------------------------------------------- */
void node_attr_init(struct node_attr* na, struct color color)
{
    na->radius = DEFAULT_NODE_RADIUS;
    na->color  = color;

    na->drag_begin_x = 0;
    na->drag_begin_y = 0;
    na->selected     = 0;
}

/* -------------------------------------------------------------------------- */
int node_attr_save(struct serializer** ser, const struct node_attr* na)
{
    int err = 0;
    err += serialize_u8(ser, na->radius);
    err += serialize_u8(ser, na->color.r);
    err += serialize_u8(ser, na->color.g);
    err += serialize_u8(ser, na->color.b);
    return err;
}
int node_attr_load(struct deserializer* des, struct node_attr* na)
{
    na->radius  = deserialize_u8(des);
    na->color.r = deserialize_u8(des);
    na->color.g = deserialize_u8(des);
    na->color.b = deserialize_u8(des);
    return deserializer_err(des);
}

/* -------------------------------------------------------------------------- */
int edge_attr_save(struct serializer** ser, const struct edge_attr* ea)
{
    int err = 0;
    err += serialize_cstr(ser, str_cstr(ea->expr_str));
    err += serialize_u8(ser, ea->color.r);
    err += serialize_u8(ser, ea->color.g);
    err += serialize_u8(ser, ea->color.b);
    return err;
}
int edge_attr_load(struct deserializer* des, struct edge_attr* ea)
{
    if (str_set_cstr(&ea->expr_str, deserialize_cstr(des)) != 0)
        return -1;
    ea->color.r = deserialize_u8(des);
    ea->color.g = deserialize_u8(des);
    ea->color.b = deserialize_u8(des);
    return deserializer_err(des);
}

/* -------------------------------------------------------------------------- */
int attrs_save(
    struct serializer** ser,
    const struct node_attr_hmap* node_attrs,
    const struct edge_attr_hmap* edge_attrs,
    const struct csfg_graph* g)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;
    int err = 0;

    if (g == NULL)
        return 0;

    err += serialize_li16(ser, csfg_graph_node_count(g));
    csfg_graph_for_each_node (g, n)
    {
        err += serialize_li32(ser, n->id);
        err += node_attr_save(ser, node_attr_hmap_find(node_attrs, n->id));
    }

    err += serialize_li16(ser, csfg_graph_edge_count(g));
    csfg_graph_for_each_edge (g, e)
    {
        err += serialize_li32(ser, e->id);
        err += edge_attr_save(ser, edge_attr_hmap_find(edge_attrs, e->id));
    }
    return err;
}

/* -------------------------------------------------------------------------- */
int attrs_load(
    struct deserializer* des,
    struct node_attr_hmap** node_attrs,
    struct edge_attr_hmap** edge_attrs,
    const struct csfg_graph* g)
{
    int count, node_id, edge_id;
    struct node_attr* na;
    struct edge_attr* ea;

    if (g == NULL)
        return 0;

    count = deserialize_li16(des);
    if (deserializer_err(des))
        return -1;
    while (count-- > 0)
    {
        node_id = deserialize_li32(des);
        switch (node_attr_hmap_emplace_or_get(node_attrs, node_id, &na))
        {
            case HMAP_OOM   : return -1;
            case HMAP_NEW   : node_attr_init(na, rgb(0, 0, 0));
            case HMAP_EXISTS: break;
        }
        if (node_attr_load(des, na) != 0)
            return -1;
    }

    count = deserialize_li16(des);
    if (deserializer_err(des))
        return -1;
    while (count-- > 0)
    {
        edge_id = deserialize_li32(des);
        switch (edge_attr_hmap_emplace_or_get(edge_attrs, edge_id, &ea))
        {
            case HMAP_OOM   : return -1;
            case HMAP_NEW   : edge_attr_init(ea, rgb(0, 0, 0));
            case HMAP_EXISTS: break;
        }
        if (edge_attr_load(des, ea) != 0)
            return -1;
    }

    return deserializer_err(des);
}

/* -------------------------------------------------------------------------- */
void attrs_clear(
    struct node_attr_hmap* node_attrs, struct edge_attr_hmap* edge_attrs)

{
    int i, id;
    struct edge_attr* ea;

    node_attr_hmap_clear(node_attrs);

    hmap_for_each (edge_attrs, i, id, ea)
    {
        (void)id, (void)ea;
        edge_attr_deinit(edge_attr_hmap_erase_slot(edge_attrs, i));
    }
    edge_attr_hmap_clear(edge_attrs);
}
