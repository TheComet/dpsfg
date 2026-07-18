#pragma once

#include "csfg/util/hmap.h"
#include "graph-editor/color.h"

struct csfg_graph;
struct str;
struct serializer;
struct deserializer;

struct node_attr
{
    struct color color;
    int radius;
    int drag_begin_x, drag_begin_y;
    unsigned selected : 1;
};

struct edge_attr
{
    struct str* expr_str;
    struct color color;
    int drag_begin_x, drag_begin_y;
    unsigned selected : 1;
};

HMAP_DECLARE(extern, node_attr_hmap, int, struct node_attr, 16)
HMAP_DECLARE(extern, edge_attr_hmap, int, struct edge_attr, 16)

void node_attr_init(struct node_attr* na, struct color color);

void edge_attr_init(struct edge_attr* ea, struct color color);
void edge_attr_deinit(struct edge_attr* ea);

int node_attr_save(struct serializer** ser, const struct node_attr* na);
int node_attr_load(struct deserializer* des, struct node_attr* na);
int edge_attr_save(struct serializer** ser, const struct edge_attr* ea);
int edge_attr_load(struct deserializer* des, struct edge_attr* ea);

int attrs_save(
    struct serializer** ser,
    const struct node_attr_hmap* node_attrs,
    const struct edge_attr_hmap* edge_attrs,
    const struct csfg_graph* g);
int attrs_load(
    struct deserializer* des,
    struct node_attr_hmap** node_attrs,
    struct edge_attr_hmap** edge_attrs,
    const struct csfg_graph* g);
void attrs_clear(
    struct node_attr_hmap* node_attrs, struct edge_attr_hmap* edge_attrs);
