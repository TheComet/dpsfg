#pragma once

struct csfg_graph;
struct csfg_node;
struct csfg_edge;
struct node_attr_hmap;
struct edge_attr_hmap;

int find_node_idx(struct csfg_graph* g, int node_id);
struct csfg_node* find_node(struct csfg_graph* g, int node_id);
int find_edge_idx(struct csfg_graph* g, int edge_id);
struct csfg_edge* find_edge(struct csfg_graph* g, int edge_id);

int find_farthest_node_in_direction(
    const struct csfg_graph* graph, double dx, double dy);
int find_nearest_node_in_direction(
    const struct csfg_graph* graph,
    int active_node_id,
    double dirx,
    double diry,
    double current_x,
    double current_y);
int find_farthest_edge_in_direction(
    const struct csfg_graph* graph, int active_edge_id, double dx, double dy);
int find_nearest_edge_in_direction(
    const struct csfg_graph* graph,
    int active_edge_id,
    double dirx,
    double diry,
    double current_x,
    double current_y);
int is_near_any_other_node(
    const struct csfg_graph* g,
    const struct csfg_node* exclude_node,
    int x,
    int y);
int is_near_any_other_edge(
    const struct csfg_graph* g,
    const struct csfg_edge* exclude_edge,
    int x,
    int y);
int node_is_connected_to_edge(struct csfg_graph* g, int n_id, int e_id);
int find_single_node_edge(const struct csfg_graph* g, int n_id);
void auto_position_node_grid(
    const struct csfg_graph* g, struct csfg_node* n, int total_node_count);
void auto_position_node_near(
    const struct csfg_graph* g, struct csfg_node* n, int near_x, int near_y);
void auto_position_edge(
    const struct csfg_graph* g,
    struct csfg_edge* e,
    const struct csfg_node* from,
    const struct csfg_node* to);
void drag_edge_connected_to_node(
    struct csfg_graph* g,
    struct csfg_edge* e,
    int n1_idx,
    int n1_prev_x,
    int n1_prev_y,
    int is_reconnect_operation);
void drag_all_edges_connected_to_node(
    struct csfg_graph* g,
    int n1_id,
    int n1_prev_x,
    int n1_prev_y,
    int is_reconnect_operation);
void bump_edge(struct csfg_graph* g, struct csfg_edge* e);
void bump_all_edges_connected_to_node(struct csfg_graph* g, int n_id);
int try_select_node(
    const struct csfg_graph* graph,
    const struct node_attr_hmap* attrs,
    int mouse_x,
    int mouse_y);
int try_select_edge(const struct csfg_graph* g, int mouse_x, int mouse_y);
