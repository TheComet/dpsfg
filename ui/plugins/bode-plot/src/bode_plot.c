#include "bode-plot/bode_plot.h"
#include "csfg/numeric/tf.h"

struct _BodePlot
{
    GtkBox     parent_instance;
    GtkWidget* drawing_area_mag;
    GtkWidget* drawing_area_phase;

    const struct csfg_tf* tf;
};

G_DEFINE_DYNAMIC_TYPE(BodePlot, bode_plot, GTK_TYPE_BOX)

/* -------------------------------------------------------------------------- */
static void draw_mag_or_phase(
    int width, int height, cairo_t* cr, BodePlot* plot, int mag_mode)
{
    struct csfg_complex y;

    double f_start, f_end, f;
    double exp_start, exp_end, exp_step, exp_;
    double val, val_min, val_max;
    double scale_x, scale_y;

    if (plot->tf == NULL)
        return;

    if (csfg_tf_interesting_frequency_interval(plot->tf, &f_start, &f_end) != 0)
        return;
    exp_start = log10(f_start);
    exp_end = log10(f_end);
    exp_step = (exp_end - exp_start) / width;

    val_min = DBL_MAX;
    val_max = -DBL_MAX;
    for (exp_ = exp_start; exp_ <= exp_end; exp_ += exp_step)
    {
        f = pow(10, exp_);
        y = csfg_tf_eval(plot->tf, csfg_complex(0.0, f));
        val = mag_mode ? csfg_complex_mag(y) : csfg_complex_phase(y);
        if (val_max < val)
            val_max = val;
        if (val_min > val)
            val_min = val;
    }
    if (mag_mode)
    {
        val_max = 20 * log10(val_max);
        val_min = 20 * log10(val_min);
    }
    scale_x = width / (exp_end - exp_start);
    scale_y = height / (val_max - val_min) * 0.45;

    cairo_translate(
        cr, -exp_start * scale_x, val_max * scale_y + height / 20.0);
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, exp_start * scale_x, 0.0);
    cairo_line_to(cr, exp_end * scale_x, 0.0);
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_set_line_width(cr, 1.0);
    for (exp_ = exp_start; exp_ <= exp_end; exp_ += exp_step)
    {
        f = pow(10, exp_);
        y = csfg_tf_eval(plot->tf, csfg_complex(0.0, f));
        val = mag_mode ? csfg_complex_mag(y) : csfg_complex_phase(y);
        if (mag_mode)
            val = 20 * log10(val);

        if (exp_ == exp_start)
            cairo_move_to(cr, exp_ * scale_x, val * -scale_y);
        else
            cairo_line_to(cr, exp_ * scale_x, val * -scale_y);
    }
    cairo_stroke(cr);
}

/* -------------------------------------------------------------------------- */
static void draw_mag_cb(
    GtkDrawingArea* area,
    cairo_t*        cr,
    int             width,
    int             height,
    gpointer        user_pointer)
{
    BodePlot* plot = user_pointer;
    draw_mag_or_phase(width, height, cr, plot, 1);
    (void)area;
}

/* -------------------------------------------------------------------------- */
static void draw_phase_cb(
    GtkDrawingArea* area,
    cairo_t*        cr,
    int             width,
    int             height,
    gpointer        user_pointer)
{

    BodePlot* plot = user_pointer;
    draw_mag_or_phase(width, height, cr, plot, 0);
    (void)area;
}

/* -------------------------------------------------------------------------- */
static void bode_plot_init(BodePlot* self)
{
    g_object_set(self, "orientation", GTK_ORIENTATION_VERTICAL, NULL);

    self->tf = NULL;

    self->drawing_area_mag = gtk_drawing_area_new();
    gtk_widget_set_hexpand(self->drawing_area_mag, TRUE);
    gtk_widget_set_size_request(self->drawing_area_mag, 1, 200);
    gtk_drawing_area_set_draw_func(
        GTK_DRAWING_AREA(self->drawing_area_mag), draw_mag_cb, self, NULL);

    self->drawing_area_phase = gtk_drawing_area_new();
    gtk_widget_set_hexpand(self->drawing_area_phase, TRUE);
    gtk_widget_set_size_request(self->drawing_area_phase, 1, 200);
    gtk_drawing_area_set_draw_func(
        GTK_DRAWING_AREA(self->drawing_area_phase), draw_phase_cb, self, NULL);

    gtk_box_append(GTK_BOX(self), self->drawing_area_mag);
    gtk_box_append(GTK_BOX(self), self->drawing_area_phase);
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
    gtk_widget_queue_draw(plot->drawing_area_mag);
    gtk_widget_queue_draw(plot->drawing_area_phase);
}
