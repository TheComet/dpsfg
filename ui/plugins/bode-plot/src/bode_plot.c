#include "bode-plot/bode_plot.h"
#include "csfg/numeric/tf.h"

struct _BodePlot
{
    GtkBox     parent_instance;
    GtkWidget* drawing_area;

    const struct csfg_tf* tf;
};

G_DEFINE_DYNAMIC_TYPE(BodePlot, bode_plot, GTK_TYPE_BOX)

/* -------------------------------------------------------------------------- */
static void draw_cb(
    GtkDrawingArea* area,
    cairo_t*        cr,
    int             width,
    int             height,
    gpointer        user_pointer)
{
    BodePlot*                  plot = user_pointer;
    const struct csfg_complex* c;
    (void)area, (void)width, (void)height;

    cairo_translate(cr, 200.0, 100.0);

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

    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    cairo_move_to(cr, -1000.0, 0.0);
    cairo_line_to(cr, 1000.0, 0.0);
    cairo_move_to(cr, 0.0, -1000.0);
    cairo_line_to(cr, 0.0, 1000.0);
    cairo_move_to(cr, 0.0, -1.0);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    if (plot->tf != NULL)
    {
    }
}

/* -------------------------------------------------------------------------- */
static void bode_plot_init(BodePlot* self)
{
    g_object_set(self, "orientation", GTK_ORIENTATION_VERTICAL, NULL);

    self->tf = NULL;

    self->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(self->drawing_area, TRUE);
    gtk_widget_set_size_request(self->drawing_area, 1, 200);
    gtk_drawing_area_set_draw_func(
        GTK_DRAWING_AREA(self->drawing_area), draw_cb, self, NULL);

    gtk_box_append(GTK_BOX(self), self->drawing_area);
}

/* -------------------------------------------------------------------------- */
static void bode_plot_finalize(GObject* obj)
{
    (void)obj;
}

/* -------------------------------------------------------------------------- */
static void bode_plot_class_init(BodePlotClass* class)
{
    GObjectClass* object_class = G_OBJECT_CLASS(class);
    object_class->finalize = bode_plot_finalize;
}

/* -------------------------------------------------------------------------- */
static void bode_plot_class_finalize(BodePlotClass* class)
{
    (void)class;
}

/* -------------------------------------------------------------------------- */
void bode_plot_register_type_internal(GTypeModule* type_module)
{
    bode_plot_register_type(type_module);
}

/* -------------------------------------------------------------------------- */
BodePlot* bode_plot_new(void)
{
    return g_object_new(PLUGIN_TYPE_BODE_PLOT, NULL);
}

/* -------------------------------------------------------------------------- */
void bode_plot_set_tf(BodePlot* plot, const struct csfg_tf* tf)
{
    plot->tf = tf;
    gtk_widget_queue_draw(plot->drawing_area);
}
