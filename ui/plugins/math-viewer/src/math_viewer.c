#include "csfg/symbolic/expr.h"
#include "csfg/util/str.h"
#include "math-viewer/math_viewer.h"

struct _MathViewer
{
    GtkBox     parent_instance;
    GtkWidget* drawing_area;

    const struct csfg_expr_pool* pool;
    int                          expr;
};

G_DEFINE_DYNAMIC_TYPE(MathViewer, math_viewer, GTK_TYPE_BOX)

/* -------------------------------------------------------------------------- */
static void draw_cb(
    GtkDrawingArea* area,
    cairo_t*        cr,
    int             width,
    int             height,
    gpointer        user_pointer)
{
    const struct csfg_node* n;
    const struct csfg_edge* e;
    MathViewer*             viewer = user_pointer;

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
    {
        struct str*           str;
        PangoLayout*          layout;
        PangoFontDescription* desc;
        int                   tw, th;
        double                tx = 0.0;
        double                ty = 0.0;

        str_init(&str);
        csfg_expr_to_str(&str, viewer->pool, viewer->expr);

        layout = pango_cairo_create_layout(cr);
        pango_layout_set_text(layout, str_cstr(str), -1);
        desc = pango_font_description_from_string("Sans 24");
        pango_layout_set_font_description(layout, desc);

        pango_layout_get_pixel_size(layout, &tw, &th);

        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_move_to(cr, tx, ty);
        pango_cairo_show_layout(cr, layout);

        g_object_unref(layout);
        pango_font_description_free(desc);
        str_deinit(str);
    }
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
    MathViewer* self = PLUGIN_MATH_VIEWER(obj);
}

/* -------------------------------------------------------------------------- */
static void math_viewer_class_init(MathViewerClass* class)
{
    GObjectClass*   object_class = G_OBJECT_CLASS(class);
    GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(class);

    object_class->finalize = math_viewer_finalize;
}

/* -------------------------------------------------------------------------- */
static void math_viewer_class_finalize(MathViewerClass* class)
{
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
    viewer->pool = pool;
    viewer->expr = expr;
    gtk_widget_queue_draw(viewer->drawing_area);
}
