#pragma once

#include "graph-editor/graph_model.h"

struct _cairo;
struct _GraphEditor;
struct _RsvgHandle;
struct csfg_graph;
struct node_attr_hmap;
struct edge_attr_hmap;

void draw_help(
    struct _cairo* cr,
    struct _RsvgHandle* svg_7steps,
    int show_7steps,
    int show_shortcuts);

void draw_grid(
    struct _cairo* cr,
    double pan_x,
    double pan_y,
    int width,
    int height,
    double zoom);

void draw_edges(
    struct _cairo* cr,
    const struct csfg_graph* g,
    const struct node_attr_hmap* node_attrs,
    const struct edge_attr_hmap* edge_attrs,
    enum graph_model_mode mode,
    int active_edge_id,
    int reconnect_edge_id);
void draw_nodes(
    struct _cairo* cr,
    const struct csfg_graph* g,
    const struct node_attr_hmap* node_attrs,
    enum graph_model_mode mode,
    int node_in_id,
    int node_out_id,
    int active_node_id,
    int selected_node_id,
    int reconnect_node_id);

void draw_drawing(
    struct _cairo* cr, const struct line_vec* drawing, double zoom);
void draw_multi_selection(
    struct _cairo* cr, double x1, double y1, double x2, double y2);
