#include "csfg/numeric/tf.h"
#include "time-plot/time_plot.h"

struct _TimePlot
{
    GtkBox     parent_instance;
    GtkWidget* drawing_area_mag;

    const struct csfg_tf* tf;
};

G_DEFINE_DYNAMIC_TYPE(TimePlot, time_plot, GTK_TYPE_BOX)

/* -------------------------------------------------------------------------- */
static void draw_mag_cb(
    GtkDrawingArea* area,
    cairo_t*        cr,
    int             width,
    int             height,
    gpointer        user_pointer)
{
    TimePlot* plot = user_pointer;
    (void)area, (void)width, (void)height;

    cairo_translate(cr, width / 2.0, height / 2.0);

    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    cairo_move_to(cr, -1000.0, 0.0);
    cairo_line_to(cr, 1000.0, 0.0);
    cairo_move_to(cr, 0.0, -1000.0);
    cairo_line_to(cr, 0.0, 1000.0);
    cairo_move_to(cr, 0.0, -1.0);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    if (plot->tf != NULL)
    {
        double value;
        double t_start, t_end, t_step, t;

        csfg_tf_interesting_time_interval(plot->tf, &t_start, &t_end);
        t_step = (t_end - t_start) / 100;

        value =
            -csfg_pfd_poly_eval_inverse_laplace(plot->tf->pfd_terms, t_start);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_move_to(cr, t_start, value * 120);

        for (t = t_start + t_step; t <= t_end; t += t_step)
        {
            value = -csfg_pfd_poly_eval_inverse_laplace(plot->tf->pfd_terms, t);
            cairo_line_to(cr, t * 4, value * 120);
        }

        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);
    }
}

/* -------------------------------------------------------------------------- */
static void time_plot_init(TimePlot* self)
{
    g_object_set(self, "orientation", GTK_ORIENTATION_VERTICAL, NULL);

    self->tf = NULL;

    self->drawing_area_mag = gtk_drawing_area_new();
    gtk_widget_set_hexpand(self->drawing_area_mag, TRUE);
    gtk_widget_set_size_request(self->drawing_area_mag, 1, 200);
    gtk_drawing_area_set_draw_func(
        GTK_DRAWING_AREA(self->drawing_area_mag), draw_mag_cb, self, NULL);

    gtk_box_append(GTK_BOX(self), self->drawing_area_mag);
}

/* -------------------------------------------------------------------------- */
static void time_plot_finalize(GObject* obj)
{
    (void)obj;
}

/* -------------------------------------------------------------------------- */
static void time_plot_class_init(TimePlotClass* class)
{
    GObjectClass* object_class = G_OBJECT_CLASS(class);
    object_class->finalize = time_plot_finalize;
}

/* -------------------------------------------------------------------------- */
static void time_plot_class_finalize(TimePlotClass* class)
{
    (void)class;
}

/* -------------------------------------------------------------------------- */
void time_plot_register_type_internal(GTypeModule* type_module)
{
    time_plot_register_type(type_module);
}

/* -------------------------------------------------------------------------- */
TimePlot* time_plot_new(void)
{
    return g_object_new(PLUGIN_TYPE_TIME_PLOT, NULL);
}

/* -------------------------------------------------------------------------- */
void time_plot_set_tf(TimePlot* plot, const struct csfg_tf* tf)
{
    plot->tf = tf;
    gtk_widget_queue_draw(plot->drawing_area_mag);
}
