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
static void expand_extents(
    const struct csfg_complex c,
    double*                   left,
    double*                   top,
    double*                   right,
    double*                   bottom)
{
    if (*left > c.real)
        *left = c.real;
    if (*right < c.real)
        *right = c.real;
    if (*top > c.imag)
        *top = c.imag;
    if (*bottom < c.imag)
        *bottom = c.imag;
}
static void calculate_tf_extents(
    const struct csfg_tf* tf,
    double*               left,
    double*               top,
    double*               right,
    double*               bottom)
{
    const struct csfg_complex* c;

    *left = DBL_MAX;
    *top = DBL_MAX;
    *right = -DBL_MAX;
    *bottom = -DBL_MAX;

    if (tf != NULL)
        vec_for_each (tf->zeros, c)
            expand_extents(*c, left, top, right, bottom);
    if (tf != NULL)
        vec_for_each (tf->poles, c)
            expand_extents(*c, left, top, right, bottom);

    /* Just make space for the unit circle */
    if (*left == DBL_MAX)
    {
        *left = -1, *right = 1;
        *top = -1, *bottom = 1;
    }
}

/* -------------------------------------------------------------------------- */
static void draw_cb(
    GtkDrawingArea* area,
    cairo_t*        cr,
    int             width,
    int             height,
    gpointer        user_pointer)
{
    double                     left, top, right, bottom;
    double                     center_x, center_y;
    double                     scale;
    PoleZeroPlot*              plot = user_pointer;
    const struct csfg_complex* c;
    (void)area;

    /* Normalize canvas to put [0,0] in center */
    if (width > height)
        scale = 1.2 / height;
    else
        scale = 1.2 / width;

    calculate_tf_extents(plot->tf, &left, &top, &right, &bottom);
    if (left > -1)
        left = -1;
    if (right < 1)
        right = 1;
    if (top > -1)
        top = -1;
    if (bottom < 1)
        bottom = 1;

    if ((right - left) > (bottom - top))
        scale *= right - left;
    else
        scale *= bottom - top;

    center_x = (right + left) / 2;
    center_y = (bottom + top) / 2;
    cairo_translate(cr, width / 2.0, height / 2.0);
    cairo_scale(cr, 1.0 / scale, 1.0 / scale);
    cairo_translate(cr, -center_x, -center_y);

    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    cairo_move_to(cr, center_x - width / 2.0, 0.0);
    cairo_line_to(cr, center_x + width / 2.0, 0.0);
    cairo_move_to(cr, 0.0, center_y - height / 2.0);
    cairo_line_to(cr, 0.0, center_y + height / 2.0);
    cairo_set_line_width(cr, scale);
    cairo_stroke(cr);

    cairo_arc(cr, 0.0, 0.0, 1.0, M_PI / 2, -M_PI / 2);
    cairo_set_line_width(cr, scale);
    cairo_stroke(cr);

    if (plot->tf != NULL)
    {
        vec_for_each (plot->tf->zeros, c)
        {
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            cairo_arc(cr, c->real, c->imag, scale * 5, 0, M_PI * 2);
            cairo_stroke(cr);
        }

        vec_for_each (plot->tf->poles, c)
        {
            double x = c->real;
            double y = c->imag;
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            cairo_move_to(cr, x - 5 * scale, y - 5 * scale);
            cairo_line_to(cr, x + 5 * scale, y + 5 * scale);
            cairo_move_to(cr, x + 5 * scale, y - 5 * scale);
            cairo_line_to(cr, x - 5 * scale, y + 5 * scale);
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
