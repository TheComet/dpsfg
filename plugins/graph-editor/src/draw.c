#include "csfg/graph/graph.h"
#include "csfg/util/str.h"
#include "graph-editor/attr.h"
#include "graph-editor/color.h"
#include "graph-editor/constants.h"
#include "graph-editor/draw.h"
#include "graph-editor/drawing.h"
#include "graph-editor/geometry.h"
#include <cairo.h>
#include <librsvg/rsvg.h>
#include <math.h>
#include <pango/pangocairo.h>

/* -------------------------------------------------------------------------- */
static void draw_grid_with_size(
    cairo_t* cr,
    double pan_x,
    double pan_y,
    int width,
    int height,
    int grid_width,
    double color)
{
    int from_x = -(int)pan_x / grid_width * grid_width - grid_width;
    int from_y = -(int)pan_y / grid_width * grid_width - grid_width;
    int to_x   = from_x + width + grid_width;
    int to_y   = from_y + height + grid_width;

    cairo_set_source_rgb(cr, color, color, color);
    for (int x = from_x; x < to_x; x += grid_width)
    {
        cairo_move_to(cr, x, from_y);
        cairo_line_to(cr, x, to_y);
    }
    for (int y = from_y; y < to_y; y += grid_width)
    {
        cairo_move_to(cr, from_x, y);
        cairo_line_to(cr, to_x, y);
    }
    cairo_stroke(cr);
}

/* -------------------------------------------------------------------------- */
void draw_grid(
    cairo_t* cr, double pan_x, double pan_y, int width, int height, double zoom)
{
    pan_x /= zoom;
    pan_y /= zoom;
    width /= zoom;
    height /= zoom;

    draw_grid_with_size(cr, pan_x, pan_y, width, height, GRID, 0.8);
    draw_grid_with_size(cr, pan_x, pan_y, width, height, GRID * 6, 0.7);
}

/* -------------------------------------------------------------------------- */
static void draw_text(
    cairo_t* cr, const char* text, double x, double y, double offset_angle)
{
    PangoLayout* layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, text, -1);
    PangoFontDescription* desc = pango_font_description_from_string("Sans 12");
    pango_layout_set_font_description(layout, desc);

    int tw, th;
    pango_layout_get_pixel_size(layout, &tw, &th);
    double tx = x - tw / 2.0;
    double ty = y - th / 2.0;

    tx += cos(offset_angle) * (tw / 2.0 + th);
    ty += sin(offset_angle) * (th / 2.0 + th);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, tx, ty);
    pango_cairo_show_layout(cr, layout);

    g_object_unref(layout);
    pango_font_description_free(desc);
}

/* -------------------------------------------------------------------------- */
static void draw_arrow(
    cairo_t* cr, double x, double y, double a, double r, double g, double b)
{
    double ox1 = 16.0, oy1 = 0.0;
    double ox2 = -8.0, oy2 = 8.0;
    double ox3 = -4.0, oy3 = 0.0;
    double ox4 = -8.0, oy4 = -8.0;

    double m00 = cos(a), m01 = -sin(a);
    double m10 = sin(a), m11 = cos(a);

    double x1 = m00 * ox1 + m01 * oy1 + x;
    double y1 = m10 * ox1 + m11 * oy1 + y;
    double x2 = m00 * ox2 + m01 * oy2 + x;
    double y2 = m10 * ox2 + m11 * oy2 + y;
    double x3 = m00 * ox3 + m01 * oy3 + x;
    double y3 = m10 * ox3 + m11 * oy3 + y;
    double x4 = m00 * ox4 + m01 * oy4 + x;
    double y4 = m10 * ox4 + m11 * oy4 + y;

    cairo_set_source_rgb(cr, r, g, b);
    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_line_to(cr, x3, y3);
    cairo_line_to(cr, x4, y4);
    cairo_close_path(cr);
    cairo_fill(cr);
}

/* -------------------------------------------------------------------------- */
static void draw_move_active_symbol(
    cairo_t* cr, double x, double y, double r, double g, double b)
{
    cairo_set_source_rgb(cr, r, g, b);
    cairo_move_to(cr, x + 12, y + 12);
    cairo_line_to(cr, x + 8, y + 8);
    cairo_move_to(cr, x - 12, y + 12);
    cairo_line_to(cr, x - 8, y + 8);
    cairo_move_to(cr, x + 12, y - 12);
    cairo_line_to(cr, x + 8, y - 8);
    cairo_move_to(cr, x - 12, y - 12);
    cairo_line_to(cr, x - 8, y - 8);
    cairo_stroke(cr);
}

/* -------------------------------------------------------------------------- */
static void draw_selected_symbol(
    cairo_t* cr,
    double x,
    double y,
    double radius,
    double r,
    double g,
    double b)
{
    radius += 2;
    cairo_set_source_rgb(cr, r, g, b);
    cairo_rectangle(cr, x - radius, y - radius, 2 * radius, 2 * radius);
    cairo_stroke(cr);
}

/* -------------------------------------------------------------------------- */
static void draw_node(
    cairo_t* cr,
    double x,
    double y,
    double radius,
    const char* name,
    double r,
    double g,
    double b,
    int is_selected,
    int is_in_node,
    int is_out_node)
{
    draw_text(cr, name, x, y, M_PI / 2);

    cairo_set_source_rgb(cr, r, g, b);
    cairo_move_to(cr, x + radius, y);
    cairo_arc(cr, x, y, radius, 0, 2 * M_PI);

    if (is_in_node || is_out_node)
        cairo_stroke(cr);
    else
        cairo_fill(cr);

    if (is_in_node)
    {
        const double radius = DEFAULT_NODE_RADIUS * 0.707;
        cairo_move_to(cr, x + radius, y + radius);
        cairo_line_to(cr, x - radius, y - radius);
        cairo_move_to(cr, x + radius, y - radius);
        cairo_line_to(cr, x - radius, y + radius);
        cairo_stroke(cr);
    }

    if (is_selected)
    {
        cairo_set_source_rgb(cr, r, g, b);
        cairo_arc(cr, x, y, radius * 1.2, 0, 2 * M_PI);
        cairo_stroke(cr);
    }
}

/* -------------------------------------------------------------------------- */
static void draw_edge(
    cairo_t* cr,
    double src_x,
    double src_y,
    double control_x,
    double control_y,
    double dst_x,
    double dst_y,
    const char* expr_str,
    double r,
    double g,
    double b)
{
    int orientation;
    double cx, cy, radius, a1, a2;
    double text_angle, arrow_angle;

    orientation = calc_circle(
        &cx, &cy, &radius, src_x, src_y, control_x, control_y, dst_x, dst_y);
    if (orientation)
    {
        a1 = atan2(src_y - cy, src_x - cx);
        a2 = atan2(dst_y - cy, dst_x - cx);
        cairo_new_path(cr);
        cairo_set_source_rgb(cr, r, g, b);
        if (orientation > 0)
            cairo_arc(cr, cx, cy, radius, a1, a2);
        else
            cairo_arc(cr, cx, cy, radius, a2, a1);
        cairo_stroke(cr);

        text_angle = atan2(cy - control_y, cx - control_x);
        arrow_angle =
            orientation < 0 ? text_angle + M_PI / 2 : text_angle - M_PI / 2;
    }
    else if (calc_circle_two_points_overlapping(
                 &cx,
                 &cy,
                 &radius,
                 src_x,
                 src_y,
                 control_x,
                 control_y,
                 dst_x,
                 dst_y))
    {
        arrow_angle = atan2(src_x - control_x, control_y - src_y);
        text_angle  = arrow_angle - M_PI / 2;
        cairo_new_path(cr);
        cairo_set_source_rgb(cr, r, g, b);
        cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);
        cairo_stroke(cr);
    }
    else
    {
        arrow_angle = atan2(dst_y - src_y, dst_x - src_x);
        text_angle  = arrow_angle + M_PI / 2;
        cairo_set_source_rgb(cr, r, g, b);
        cairo_move_to(cr, src_x, src_y);
        cairo_line_to(cr, dst_x, dst_y);
        cairo_stroke(cr);
    }

    if (text_angle < M_PI)
        text_angle += M_PI;
    draw_arrow(cr, control_x, control_y, arrow_angle, r, g, b);
    draw_text(cr, expr_str, control_x, control_y, text_angle);
}

/* -------------------------------------------------------------------------- */
void draw_help(
    cairo_t* cr, RsvgHandle* svg_7steps, int show_7steps, int show_shortcuts)
{
    RsvgRectangle viewport = {20, 20, 136, 334};
    if (show_7steps)
        rsvg_handle_render_document(svg_7steps, cr, &viewport, NULL);
    if (show_shortcuts)
        draw_text(cr, "(TODO)", 0, show_7steps ? 380 : 20, 0);
}

/* -------------------------------------------------------------------------- */
void
draw_drawing(cairo_t* cr, const struct line_vec* drawing, double zoom)
{
    int i;
    const struct line* line;
    const struct point* point;
    vec_for_each (drawing, line)
    {
        cairo_set_source_rgb(
            cr,
            line->color.r / 255.0,
            line->color.g / 255.0,
            line->color.b / 255.0);
        cairo_set_line_width(cr, (line->selected ? 5.0 : 3.0) / zoom);
        vec_enumerate (line->points, i, point)
        {
            if (i == 0)
                cairo_move_to(cr, point->x, point->y);
            else
                cairo_line_to(cr, point->x, point->y);
        }
        cairo_stroke(cr);
    }
}

/* -------------------------------------------------------------------------- */
void
draw_multi_selection(cairo_t* cr, double x1, double y1, double x2, double y2)
{
    cairo_set_source_rgb(cr, 0.8, 0.5, 0.0);
    cairo_rectangle(cr, x1, y1, x2 - x1, y2 - y1);
    cairo_stroke(cr);
}

/* -------------------------------------------------------------------------- */
void draw_edges(
    cairo_t* cr,
    const struct csfg_graph* g,
    const struct node_attr_hmap* node_attrs,
    const struct edge_attr_hmap* edge_attrs,
    enum graph_model_mode mode,
    int active_edge_id,
    int reconnect_edge_id)
{
    struct color color;
    const struct csfg_edge* e;
    const struct csfg_node *n_src, *n_dst;
    const struct edge_attr* ea;
    const struct node_attr *na_src, *na_dst;

    csfg_graph_for_each_edge (g, e)
    {
        n_src  = csfg_graph_get_node(g, e->n_idx_from);
        n_dst  = csfg_graph_get_node(g, e->n_idx_to);
        ea     = edge_attr_hmap_find(edge_attrs, e->id);
        na_src = node_attr_hmap_find(node_attrs, n_src->id);
        na_dst = node_attr_hmap_find(node_attrs, n_dst->id);
        if (ea == NULL || na_src == NULL || na_dst == NULL)
            continue;
        color = ea->color;
        if (e->id == active_edge_id)
            color = highlight_color(ea->color);
        if (e->id == reconnect_edge_id)
            color = highlight_color(ea->color);
        draw_edge(
            cr,
            n_src->x,
            n_src->y,
            e->x,
            e->y,
            n_dst->x,
            n_dst->y,
            str_cstr(ea->expr_str),
            color.r / 255.0,
            color.g / 255.0,
            color.b / 255.0);
        if ((mode == MODE_MOVE && e->id == active_edge_id) ||
            (mode == MODE_RECONNECT_FROM && e->id == reconnect_edge_id))
        {
            draw_move_active_symbol(
                cr,
                e->x,
                e->y,
                color.r / 255.0,
                color.g / 255.0,
                color.b / 255.0);
        }
    }
}

/* -------------------------------------------------------------------------- */
void draw_nodes(
    cairo_t* cr,
    const struct csfg_graph* g,
    const struct node_attr_hmap* node_attrs,
    enum graph_model_mode mode,
    int node_in_id,
    int node_out_id,
    int active_node_id,
    int selected_node_id,
    int reconnect_node_id)
{
    struct color color;
    const struct csfg_node* n;
    const struct node_attr* na;

    csfg_graph_for_each_node (g, n)
    {
        na = node_attr_hmap_find(node_attrs, n->id);
        if (na == NULL)
            continue;
        color = na->color;
        if (n->id == active_node_id)
            color = highlight_color(na->color);
        if (n->id == reconnect_node_id)
            color = highlight_color(na->color);
        draw_node(
            cr,
            n->x,
            n->y,
            na->radius,
            str_cstr(n->name),
            color.r / 255.0,
            color.g / 255.0,
            color.b / 255.0,
            n->id == selected_node_id,
            n->id == node_in_id,
            n->id == node_out_id);
        if ((mode == MODE_MOVE && n->id == active_node_id) ||
            (mode == MODE_RECONNECT_FROM && n->id == reconnect_node_id))
        {
            draw_move_active_symbol(
                cr,
                n->x,
                n->y,
                color.r / 255.0,
                color.g / 255.0,
                color.b / 255.0);
        }
        if (na->selected)
        {
            draw_selected_symbol(
                cr,
                n->x,
                n->y,
                na->radius,
                color.r / 255.0,
                color.g / 255.0,
                color.b / 255.0);
        }
    }
}
