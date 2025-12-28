#include "csfg/numeric/tf.h"
#include "time-plot/time_plot.h"

struct _TimePlot
{
    GtkBox     parent_instance;
    GtkWidget* drawing_area;

    const struct csfg_tf*       tf;
    const struct csfg_pfd_poly* pfd_impulse;
    const struct csfg_pfd_poly* pfd_step;
    const struct csfg_pfd_poly* pfd_ramp;

    unsigned enable_impulse : 1;
    unsigned enable_step : 1;
    unsigned enable_ramp : 1;
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

    if (plot->tf != NULL)
    {
        int    exponent, i;
        double scale_x, scale_y;
        double value;
        double t_start, t_end, t_step, t;
        double y_min, y_max;
        double max_abs_value = 0;

        if (csfg_tf_interesting_time_interval(plot->tf, &t_start, &t_end) != 0)
            return;
        t_step = (t_end - t_start) / width * 0.9;

        y_min = DBL_MAX;
        y_max = -DBL_MAX;
        for (t = t_start; t <= t_end; t += t_step)
        {
            if (plot->enable_impulse)
            {
                value =
                    csfg_pfd_poly_eval_inverse_laplace(plot->pfd_impulse, t);
                if (y_max < value)
                    y_max = value;
                if (y_min > value)
                    y_min = value;
                value = fabs(value);
                if (max_abs_value < value)
                    max_abs_value = value;
            }

            if (plot->enable_step)
            {
                value = csfg_pfd_poly_eval_inverse_laplace(plot->pfd_step, t);
                if (y_max < value)
                    y_max = value;
                if (y_min > value)
                    y_min = value;
                value = fabs(value);
                if (max_abs_value < value)
                    max_abs_value = value;
            }

            if (plot->enable_ramp)
            {
                value = csfg_pfd_poly_eval_inverse_laplace(plot->pfd_ramp, t);
                if (y_max < value)
                    y_max = value;
                if (y_min > value)
                    y_min = value;
                value = fabs(value);
                if (max_abs_value < value)
                    max_abs_value = value;
            }
        }
        scale_x = width / (t_end - t_start) * 0.95;
        scale_y = height / max_abs_value * 0.45;

        cairo_translate(cr, width / 20.0, height / 2.0);
        cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, -1000.0, 0.0);
        cairo_line_to(cr, 1000.0, 0.0);
        cairo_move_to(cr, 0.0, -1000.0);
        cairo_line_to(cr, 0.0, 1000.0);
        cairo_stroke(cr);

        if (plot->enable_impulse)
        {
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            cairo_set_line_width(cr, 1.0);
            for (t = t_start; t <= t_end; t += t_step)
            {
                value =
                    csfg_pfd_poly_eval_inverse_laplace(plot->pfd_impulse, t);
                if (t == t_start)
                    cairo_move_to(cr, t * scale_x, value * -scale_y);
                else
                    cairo_line_to(cr, t * scale_x, value * -scale_y);
            }
            cairo_stroke(cr);
        }

        if (plot->enable_step)
        {
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            cairo_set_line_width(cr, 1.0);
            for (t = t_start; t <= t_end; t += t_step)
            {
                value = csfg_pfd_poly_eval_inverse_laplace(plot->pfd_step, t);
                if (t == t_start)
                    cairo_move_to(cr, t * scale_x, value * -scale_y);
                else
                    cairo_line_to(cr, t * scale_x, value * -scale_y);
            }
            cairo_stroke(cr);
        }

        if (plot->enable_ramp)
        {
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            cairo_set_line_width(cr, 1.0);
            for (t = t_start; t <= t_end; t += t_step)
            {
                value = csfg_pfd_poly_eval_inverse_laplace(plot->pfd_ramp, t);
                if (t == t_start)
                    cairo_move_to(cr, t * scale_x, value * -scale_y);
                else
                    cairo_line_to(cr, t * scale_x, value * -scale_y);
            }
            cairo_stroke(cr);
        }

        cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, -5, y_max * -scale_y);
        cairo_line_to(cr, 5, y_max * -scale_y);
        cairo_move_to(cr, -5, y_min * -scale_y);
        cairo_line_to(cr, 5, y_min * -scale_y);
        cairo_stroke(cr);

        cairo_set_line_width(cr, 1.0);
        cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
        exponent = (int)ceil(log10(t_end));
        t = pow(10, exponent);
        cairo_move_to(cr, t * scale_x, 5);
        cairo_line_to(cr, t * scale_x, -5);
        t = pow(10, exponent - 1);
        cairo_move_to(cr, t * scale_x, 5);
        cairo_line_to(cr, t * scale_x, -5);
        t = pow(10, exponent - 2);
        cairo_move_to(cr, t * scale_x, 5);
        cairo_line_to(cr, t * scale_x, -5);

        t = pow(10, exponent);
        t_step = t / 100;
        for (i = 0; i != 100; ++i)
        {
            double height = i % 10 == 0 ? 5 : 2;
            cairo_move_to(cr, t * scale_x, height);
            cairo_line_to(cr, t * scale_x, -height);
            t -= t_step;
        }
        cairo_stroke(cr);
    }
}

/* -------------------------------------------------------------------------- */
static void on_impulse_toggled(GtkCheckButton* button, gpointer user_pointer)
{
    TimePlot* self = user_pointer;
    self->enable_impulse =
        gtk_check_button_get_active(GTK_CHECK_BUTTON(button)) ? 1 : 0;
    gtk_widget_queue_draw(self->drawing_area);
}
static void on_step_toggled(GtkCheckButton* button, gpointer user_pointer)
{
    TimePlot* self = user_pointer;
    self->enable_step =
        gtk_check_button_get_active(GTK_CHECK_BUTTON(button)) ? 1 : 0;
    gtk_widget_queue_draw(self->drawing_area);
}
static void on_ramp_toggled(GtkCheckButton* button, gpointer user_pointer)
{
    TimePlot* self = user_pointer;
    self->enable_ramp =
        gtk_check_button_get_active(GTK_CHECK_BUTTON(button)) ? 1 : 0;
    gtk_widget_queue_draw(self->drawing_area);
}

/* -------------------------------------------------------------------------- */
static void time_plot_init(TimePlot* self)
{
    GtkWidget* checkboxes_box;
    GtkWidget* check_impulse;
    GtkWidget* check_step;
    GtkWidget* check_ramp;

    g_object_set(self, "orientation", GTK_ORIENTATION_VERTICAL, NULL);

    self->tf = NULL;
    self->enable_impulse = 0;
    self->enable_step = 1;
    self->enable_ramp = 0;

    self->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(self->drawing_area, TRUE);
    gtk_widget_set_size_request(self->drawing_area, 1, 200);
    gtk_drawing_area_set_draw_func(
        GTK_DRAWING_AREA(self->drawing_area), draw_mag_cb, self, NULL);

    check_impulse = gtk_check_button_new_with_label("Impulse");
    check_step = gtk_check_button_new_with_label("Step");
    check_ramp = gtk_check_button_new_with_label("Ramp");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check_step), TRUE);
    g_signal_connect(
        check_impulse, "toggled", G_CALLBACK(on_impulse_toggled), self);
    g_signal_connect(check_step, "toggled", G_CALLBACK(on_step_toggled), self);
    g_signal_connect(check_ramp, "toggled", G_CALLBACK(on_ramp_toggled), self);

    checkboxes_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(checkboxes_box), check_impulse);
    gtk_box_append(GTK_BOX(checkboxes_box), check_step);
    gtk_box_append(GTK_BOX(checkboxes_box), check_ramp);

    gtk_box_append(GTK_BOX(self), checkboxes_box);
    gtk_box_append(GTK_BOX(self), self->drawing_area);
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
}
void time_plot_set_impulse(TimePlot* plot, const struct csfg_pfd_poly* pfd)
{
    plot->pfd_impulse = pfd;
}
void time_plot_set_step(TimePlot* plot, const struct csfg_pfd_poly* pfd)
{
    plot->pfd_step = pfd;
}
void time_plot_set_ramp(TimePlot* plot, const struct csfg_pfd_poly* pfd)
{
    plot->pfd_ramp = pfd;
    gtk_widget_queue_draw(plot->drawing_area);
}
