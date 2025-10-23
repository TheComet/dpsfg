#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/tf_expr.h"
#include "csfg/util/str.h"
#include "math-viewer/math_viewer.h"

struct _MathViewer
{
    GtkBox     parent_instance;
    GtkWidget* drawing_area;

    const struct csfg_tf_expr*   tf;
    const struct csfg_expr_pool* pool;
    int                          expr;
};

G_DEFINE_DYNAMIC_TYPE(MathViewer, math_viewer, GTK_TYPE_BOX)

/* -------------------------------------------------------------------------- */
static void draw_expr(cairo_t* cr, const struct csfg_expr_pool* pool, int expr)
{
    struct str*           str;
    PangoLayout*          layout;
    PangoFontDescription* desc;
    int                   tw1, th1;

    str_init(&str);
    cairo_set_source_rgb(cr, 0, 0, 0);

    layout = pango_cairo_create_layout(cr);
    desc = pango_font_description_from_string("Sans 16");
    pango_layout_set_font_description(layout, desc);

    csfg_expr_to_str(&str, pool, expr);
    pango_layout_set_text(layout, str_cstr(str), -1);
    cairo_move_to(cr, 0.0, 0.0);
    pango_cairo_show_layout(cr, layout);
    pango_layout_get_pixel_size(layout, &tw1, &th1);

    g_object_unref(layout);
    pango_font_description_free(desc);
    str_deinit(str);
}

/* -------------------------------------------------------------------------- */
static void draw_tf(
    cairo_t*                     cr,
    const struct csfg_expr_pool* pool,
    const struct csfg_tf_expr*   tf)
{
    struct str*           str;
    PangoLayout*          layout;
    PangoFontDescription* desc;
    int                   tw, th;
    int                   widest;
    double                tx = 0.0;
    double                ty = 0.0;

    str_init(&str);
    cairo_set_source_rgb(cr, 0, 0, 0);

    layout = pango_cairo_create_layout(cr);
    desc = pango_font_description_from_string("Sans 16");
    pango_layout_set_font_description(layout, desc);

    csfg_poly_expr_to_str(&str, pool, tf->num);
    pango_layout_set_text(layout, str_cstr(str), -1);
    cairo_move_to(cr, tx, ty);
    pango_cairo_show_layout(cr, layout);
    pango_layout_get_pixel_size(layout, &tw, &th);
    widest = tw;

    str_clear(str);
    csfg_poly_expr_to_str(&str, pool, tf->den);
    pango_layout_set_text(layout, str_cstr(str), -1);
    cairo_move_to(cr, tx, ty + th);
    pango_cairo_show_layout(cr, layout);
    pango_layout_get_pixel_size(layout, &tw, &th);
    widest = tw > widest ? tw : widest;

    cairo_move_to(cr, 0.0, th + 1.0);
    cairo_line_to(cr, widest, th + 1.0);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    g_object_unref(layout);
    pango_font_description_free(desc);
    str_deinit(str);
}

/* -------------------------------------------------------------------------- */
static void draw_cb(
    GtkDrawingArea* area,
    cairo_t*        cr,
    int             width,
    int             height,
    gpointer        user_pointer)
{
    MathViewer* viewer = user_pointer;
    (void)area, (void)width, (void)height;

    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    for (int x = -1000; x < 1000; x += 20)
    {
        cairo_move_to(cr, x, -1000);
        cairo_line_to(cr, x, 1000);
    }
    for (int y = -1000; y < 1000; y += 20)
    {
        cairo_move_to(cr, -1000, y);
        cairo_line_to(cr, 1000, y);
    }
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    if (viewer->pool != NULL && viewer->expr > -1)
        draw_expr(cr, viewer->pool, viewer->expr);
    if (viewer->pool != NULL && viewer->tf != NULL)
        draw_tf(cr, viewer->pool, viewer->tf);
}

/* -------------------------------------------------------------------------- */
static void math_viewer_init(MathViewer* self)
{
    self->pool = NULL;
    self->pool = NULL;
    self->expr = -1;
    self->expr = -1;

    g_object_set(self, "orientation", GTK_ORIENTATION_VERTICAL, NULL);

    self->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(self->drawing_area, TRUE);
    gtk_widget_set_size_request(self->drawing_area, 1, 200);
    gtk_drawing_area_set_draw_func(
        GTK_DRAWING_AREA(self->drawing_area), draw_cb, self, NULL);

    gtk_box_append(GTK_BOX(self), self->drawing_area);
}

/* -------------------------------------------------------------------------- */
static void math_viewer_finalize(GObject* obj)
{
    (void)obj;
}

/* -------------------------------------------------------------------------- */
static void math_viewer_class_init(MathViewerClass* class)
{
    GObjectClass* object_class = G_OBJECT_CLASS(class);
    object_class->finalize = math_viewer_finalize;
}

/* -------------------------------------------------------------------------- */
static void math_viewer_class_finalize(MathViewerClass* class)
{
    (void)class;
}

/* -------------------------------------------------------------------------- */
void math_viewer_register_type_internal(GTypeModule* type_module)
{
    math_viewer_register_type(type_module);
}

/* -------------------------------------------------------------------------- */
MathViewer* math_viewer_new(void)
{
    return g_object_new(PLUGIN_TYPE_MATH_VIEWER, NULL);
}

/* -------------------------------------------------------------------------- */
void math_viewer_set_expr(
    MathViewer* viewer, const struct csfg_expr_pool* pool, int expr)
{
    viewer->tf = NULL;
    viewer->pool = pool;
    viewer->expr = expr;
    gtk_widget_queue_draw(viewer->drawing_area);
}

/* -------------------------------------------------------------------------- */
void math_viewer_set_tf(
    MathViewer*                  viewer,
    const struct csfg_expr_pool* pool,
    const struct csfg_tf_expr*   tf)
{
    viewer->tf = tf;
    viewer->pool = pool;
    viewer->expr = -1;
    gtk_widget_queue_draw(viewer->drawing_area);
}
