#include "csfg/util/log.h"
#include "dpsfg/editable_label.h"

struct _DPSFGEditableLabel
{
    GtkBox parent_instance;

    GtkWidget* stack;
    GtkWidget* label;
    GtkWidget* text;
    gboolean is_editing;
};

struct _DPSFGEditableLabelClass
{
    GtkBoxClass parent_class;
};

enum
{
    SIGNAL_TEXT_CHANGED,
    SIGNAL_COUNT
};

static gint signals[SIGNAL_COUNT];

G_DEFINE_TYPE(DPSFGEditableLabel, dpsfg_editable_label, GTK_TYPE_BOX)

/* -------------------------------------------------------------------------- */
static void start_editing(DPSFGEditableLabel* self)
{
    const char* current_text = gtk_label_get_text(GTK_LABEL(self->label));
    gtk_editable_set_text(GTK_EDITABLE(self->text), current_text);
    gtk_stack_set_visible_child(GTK_STACK(self->stack), self->text);
    gtk_widget_grab_focus(self->text);
    gtk_editable_select_region(GTK_EDITABLE(self->text), 0, -1);
    self->is_editing = TRUE;
}

/* -------------------------------------------------------------------------- */
static void finish_editing(DPSFGEditableLabel* self, gboolean commit)
{
    const char* current_text = gtk_label_get_text(GTK_LABEL(self->label));
    const char* new_text     = gtk_editable_get_text(GTK_EDITABLE(self->text));
    gboolean changed         = g_strcmp0(current_text, new_text) != 0;
    if (commit && changed)
    {
        gtk_label_set_text(GTK_LABEL(self->label), new_text);
        g_signal_emit(self, signals[SIGNAL_TEXT_CHANGED], 0);
    }
    gtk_stack_set_visible_child(GTK_STACK(self->stack), self->label);
    self->is_editing = FALSE;
}

/* -------------------------------------------------------------------------- */
static gboolean
toggle_editing_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    DPSFGEditableLabel* self = user_data;
    (void)widget, (void)unused;
    if (self->is_editing)
        finish_editing(self, TRUE);
    else
        start_editing(self);
    return TRUE;
}
static gboolean
cancel_editing_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    DPSFGEditableLabel* self = user_data;
    (void)widget, (void)unused;
    if (self->is_editing)
        finish_editing(self, FALSE);
    return TRUE;
}
void pressed_cb(
    GtkGestureClick* gesture,
    gint n_press,
    gdouble x,
    gdouble y,
    gpointer user_data)
{
    DPSFGEditableLabel* self = user_data;
    (void)gesture, (void)x, (void)y;
    if (n_press == 2 && !self->is_editing)
        start_editing(self);
}
static void text_activate_cb(GtkText* text, gpointer user_data)
{
    DPSFGEditableLabel* self = user_data;
    (void)text;
    if (self->is_editing)
        finish_editing(self, TRUE);
    else
        start_editing(self);
}

/* -------------------------------------------------------------------------- */
static void dpsfg_editable_label_init(DPSFGEditableLabel* self)
{
    GtkShortcutTrigger* trigger;
    GtkShortcutAction* action;
    GtkEventController* controller;
    GtkGesture* gesture;

    controller = gtk_shortcut_controller_new();

    trigger = gtk_keyval_trigger_new(GDK_KEY_Return, GDK_NO_MODIFIER_MASK);
    action  = gtk_callback_action_new(toggle_editing_cb, self, NULL);
    gtk_shortcut_controller_add_shortcut(
        GTK_SHORTCUT_CONTROLLER(controller), gtk_shortcut_new(trigger, action));

    trigger = gtk_keyval_trigger_new(GDK_KEY_Escape, GDK_NO_MODIFIER_MASK);
    action  = gtk_callback_action_new(cancel_editing_cb, self, NULL);
    gtk_shortcut_controller_add_shortcut(
        GTK_SHORTCUT_CONTROLLER(controller), gtk_shortcut_new(trigger, action));

    gtk_widget_add_controller(GTK_WIDGET(self), controller);

    gesture = gtk_gesture_click_new();
    g_signal_connect(gesture, "pressed", G_CALLBACK(pressed_cb), self);
    gtk_widget_add_controller(GTK_WIDGET(self), GTK_EVENT_CONTROLLER(gesture));

    self->label = gtk_label_new("");
    self->text  = gtk_text_new();
    self->stack = gtk_stack_new();
    gtk_stack_add_child(GTK_STACK(self->stack), self->label);
    gtk_stack_add_child(GTK_STACK(self->stack), self->text);
    gtk_widget_set_focusable(self->stack, FALSE);

    gtk_box_append(GTK_BOX(self), self->stack);
    gtk_widget_set_focusable(GTK_WIDGET(self), FALSE);

    g_signal_connect(
        self->text, "activate", G_CALLBACK(text_activate_cb), self);
}

/* -------------------------------------------------------------------------- */
static void dpsfg_editable_label_class_init(DPSFGEditableLabelClass* class)
{
    GObjectClass* object_class   = G_OBJECT_CLASS(class);
    signals[SIGNAL_TEXT_CHANGED] = g_signal_new(
        "text-changed",
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
GtkWidget* dpsfg_editable_label_new(void)
{
    return g_object_new(DPSFG_TYPE_EDITABLE_LABEL, NULL);
}

/* -------------------------------------------------------------------------- */
const char* dpsfg_editable_label_get_text(DPSFGEditableLabel* self)
{
    return gtk_label_get_text(GTK_LABEL(self->label));
}

/* -------------------------------------------------------------------------- */
void dpsfg_editable_label_set_text(DPSFGEditableLabel* self, const char* text)
{
    gtk_label_set_text(GTK_LABEL(self->label), text);
}
