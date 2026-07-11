#include "csfg/util/log.h"
#include "graph-editor/color_picker.h"

struct _ColorPicker
{
    GtkToggleButton parent_instance;
    GtkColorDialog* color_dialog;
    GtkWidget* swatch;
    GdkRGBA color;
};

G_DEFINE_DYNAMIC_TYPE(ColorPicker, color_picker, GTK_TYPE_TOGGLE_BUTTON)

enum
{
    SIGNAL_COLOR_SELECTED,
    SIGNAL_COUNT
};

static gint signals[SIGNAL_COUNT];

/* -------------------------------------------------------------------------- */
static void
color_chosen(GObject* source, GAsyncResult* result, gpointer user_data)
{
    ColorPicker* picker = user_data;
    GError* error       = NULL;
    GdkRGBA* rgba       = gtk_color_dialog_choose_rgba_finish(
        GTK_COLOR_DIALOG(source), result, &error);

    if (rgba != NULL)
    {
        picker->color = *rgba;
        g_signal_emit(picker, signals[SIGNAL_COLOR_SELECTED], 0, picker->color);
        gdk_rgba_free(rgba);
        gtk_widget_queue_draw(picker->swatch);
    }

    if (error != NULL)
        g_error_free(error);
}

/* -------------------------------------------------------------------------- */
static void right_click(
    GtkGestureClick* gesture,
    gint n_press,
    gdouble x,
    gdouble y,
    gpointer user_data)
{
    ColorPicker* picker = user_data;
    (void)gesture, (void)n_press, (void)x, (void)y;

    gtk_color_dialog_choose_rgba(
        picker->color_dialog,
        GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(picker))),
        &picker->color,
        NULL,
        color_chosen,
        picker);
}

/* -------------------------------------------------------------------------- */
static void button_toggled(GtkButton* button, gpointer user_data)
{
    ColorPicker* picker = user_data;
    (void)button;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(picker)))
        g_signal_emit(picker, signals[SIGNAL_COLOR_SELECTED], 0, picker->color);
}

/* -------------------------------------------------------------------------- */
static void draw_swatch(
    GtkDrawingArea* area,
    cairo_t* cr,
    int width,
    int height,
    gpointer user_data)
{
    ColorPicker* picker = user_data;
    (void)area;

    gdk_cairo_set_source_rgba(cr, &picker->color);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
}

/* -------------------------------------------------------------------------- */
static void color_picker_init(ColorPicker* self)
{
    self->swatch = gtk_drawing_area_new();
    gtk_widget_set_size_request(self->swatch, 24, 24);
    gtk_drawing_area_set_draw_func(
        GTK_DRAWING_AREA(self->swatch), draw_swatch, self, NULL);
    gtk_button_set_child(GTK_BUTTON(self), self->swatch);

    self->color_dialog = gtk_color_dialog_new();

    GtkGesture* gesture = gtk_gesture_click_new();
    gtk_gesture_single_set_button(
        GTK_GESTURE_SINGLE(gesture), GDK_BUTTON_SECONDARY);
    gtk_widget_add_controller(GTK_WIDGET(self), GTK_EVENT_CONTROLLER(gesture));

    g_signal_connect(self, "toggled", G_CALLBACK(button_toggled), self);
    g_signal_connect(gesture, "released", G_CALLBACK(right_click), self);
}

/* -------------------------------------------------------------------------- */
static void color_picker_finalize(GObject* object)
{
    ColorPicker* picker = PLUGIN_COLOR_PICKER(object);
    g_object_unref(picker->color_dialog);
    G_OBJECT_CLASS(color_picker_parent_class)->finalize(object);
}

/* -------------------------------------------------------------------------- */
static void color_picker_class_init(ColorPickerClass* class)
{
    GObjectClass* object_class = G_OBJECT_CLASS(class);
    object_class->finalize     = color_picker_finalize;

    signals[SIGNAL_COLOR_SELECTED] = g_signal_new(
        "color-selected",
        G_OBJECT_CLASS_TYPE(object_class),
        G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
        0,
        NULL,
        NULL,
        NULL,
        G_TYPE_NONE,
        0);
}

/* -------------------------------------------------------------------------- */
static void color_picker_class_finalize(ColorPickerClass* class)
{
    (void)class;
}

/* -------------------------------------------------------------------------- */
void color_picker_register_type_internal(GTypeModule* type_module)
{
    color_picker_register_type(type_module);
}

/* -------------------------------------------------------------------------- */
GtkWidget* color_picker_new(GdkRGBA color)
{
    ColorPicker* picker = g_object_new(PLUGIN_TYPE_COLOR_PICKER, NULL);
    picker->color       = color;
    return GTK_WIDGET(picker);
}

/* -------------------------------------------------------------------------- */
GdkRGBA color_picker_get_color(ColorPicker* picker)
{
    return picker->color;
}
