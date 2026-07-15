#pragma once

#include "graph-editor/color.h"
#include <stdint.h>

struct csfg_graph;
struct deserializer;
struct edge_attr_hmap;
struct line_vec;
struct node_attr_hmap;
struct plugin_ctx;
struct plugin_notify_interface;
struct plugin_notify_context;
struct serializer;

enum graph_model_mode
{
    MODE_NORMAL,
    MODE_MOVE,
    MODE_RECONNECT_FROM,
    MODE_RECONNECT_TO,
    MODE_DRAW,
};

struct graph_model
{
    const struct plugin_notify_interface* icb;
    struct plugin_notify_context* cb;
    struct plugin_ctx* plugin_ctx;

    struct csfg_graph* graph;
    struct node_attr_hmap* node_attrs;
    struct edge_attr_hmap* edge_attrs;
    struct undo_stack_vec* undo_stack;
    struct line_vec* drawing;

    struct color graph_color;
    struct color draw_color;

    int undo_stack_ptr;
    int active_node_id;
    int active_edge_id;
    int marked_node_id;
    int reconnect_node_id;
    int reconnect_edge_id;
    int node_in_id;
    int node_out_id;

    enum graph_model_mode mode;
};

void select_next_active_node_in_direction(
    struct graph_model* model, double dx, double dy);
void select_next_active_edge_in_direction(
    struct graph_model* model, double dx, double dy);
void select_next_connected_node_in_direction(
    struct graph_model* model, int edge_id, double dirx, double diry);
void select_next_connected_node_in_direction(
    struct graph_model* model, int edge_id, double dirx, double diry);
void select_next_connected_edge_in_direction(
    struct graph_model* model, int node_id, double dirx, double diry);
void select_nearest_connected_node(struct graph_model* model, int edge_id);
void select_nearest_connected_edge(struct graph_model* model, int node_id);
int reconnect_edge(
    struct graph_model* model,
    int edge_id,
    int current_node_id,
    int target_node_id);
void move_active_node_in_direction(
    struct graph_model* model, double dx, double dy);
void move_active_edge_in_direction(
    struct graph_model* model, double dx, double dy);
int create_new_edge(
    struct graph_model* model, int n_id_selected, int n_id_active);
int create_new_node(struct graph_model* model, int n_id_prev);
int delete_edge(struct graph_model* model, int edge_id);
int delete_node(struct graph_model* model, int node_id);
void delete_multi_selection_objects(struct graph_model* model);
void multi_select_nodes_and_lines(
    struct graph_model* model, int x1, int y1, int x2, int y2, int append);
void multi_deselect_all(struct graph_model* model);
void flip_active_edge(struct graph_model* model);
void recolor_selected_objects(struct graph_model* model, struct color color);

void graph_model_init(
    struct graph_model* model,
    struct plugin_ctx* plugin_ctx,
    const struct plugin_notify_interface* icb,
    struct plugin_notify_context* cb);
void graph_model_deinit(struct graph_model* model);

void graph_model_set_graph(
    struct graph_model* model, struct csfg_graph* g, int node_in, int node_out);
void graph_model_clear_graph(struct graph_model* model);
void notify_graph_changed(struct graph_model* model);
void graph_model_rebuild_graph(
    struct graph_model* model, int node_in, int node_out);
