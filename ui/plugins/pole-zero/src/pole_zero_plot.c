#include "csfg/numeric/tf.h"
#include "pole-zero/pole_zero_plot.h"

struct _PoleZeroPlot
{
    GtkBox     parent_instance;
    GtkWidget* drawing_area;

    const struct csfg_tf* tf;
};

G_DEFINE_DYNAMIC_TYPE(PoleZeroPlot, pole_zero_plot, GTK_TYPE_BOX)

/* -------------------------------------------------------------------------- */
static void draw_cb(
    GtkDrawingArea* area,
    cairo_t*        cr,
    int             width,
    int             height,
    gpointer        user_pointer)
{
    PoleZeroPlot*              plot = user_pointer;
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
    cairo_arc(cr, 0.0, 0.0, 60.0, M_PI / 2, -M_PI / 2);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    if (plot->tf != NULL)
    {
        vec_for_each (plot->tf->zeros, c)
        {
            double x = c->real * 60.0;
            double y = c->imag * 60.0;
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            cairo_arc(cr, x, y, 5.0, 0, M_PI * 2);
            cairo_stroke(cr);
        }

        vec_for_each (plot->tf->poles, c)
        {
            double x = c->real * 60.0;
            double y = c->imag * 60.0;
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            cairo_move_to(cr, x - 5, y - 5);
            cairo_line_to(cr, x + 5, y + 5);
            cairo_move_to(cr, x + 5, y - 5);
            cairo_line_to(cr, x - 5, y + 5);
            cairo_stroke(cr);
        }
    }
}

/* -------------------------------------------------------------------------- */
static void pole_zero_plot_init(PoleZeroPlot* self)
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
static void pole_zero_plot_finalize(GObject* obj)
{
    (void)obj;
}

/* -------------------------------------------------------------------------- */
static void pole_zero_plot_class_init(PoleZeroPlotClass* class)
{
    GObjectClass* object_class = G_OBJECT_CLASS(class);
    object_class->finalize = pole_zero_plot_finalize;
}

/* -------------------------------------------------------------------------- */
static void pole_zero_plot_class_finalize(PoleZeroPlotClass* class)
{
    (void)class;
}

/* -------------------------------------------------------------------------- */
void pole_zero_plot_register_type_internal(GTypeModule* type_module)
{
    pole_zero_plot_register_type(type_module);
}

/* -------------------------------------------------------------------------- */
PoleZeroPlot* pole_zero_plot_new(void)
{
    return g_object_new(PLUGIN_TYPE_POLE_ZERO_PLOT, NULL);
}

/* -------------------------------------------------------------------------- */
void pole_zero_plot_set_tf(PoleZeroPlot* plot, const struct csfg_tf* tf)
{
    plot->tf = tf;
    gtk_widget_queue_draw(plot->drawing_area);
}
