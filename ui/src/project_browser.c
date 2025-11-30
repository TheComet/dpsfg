#include "csfg/util/tracker.h"
#include "dpsfg/project_browser.h"

#define COLUMNS_LIST                                                           \
    X(GROUP, center_label, column_n, "Group")                                  \
    X(DATE, column1, column_1, "Date")                                         \
    X(TIME, column1, column_1, "Time")                                         \
    X(NAME, center_label, column_n, "Name")

enum column
{
#define X(name, setup, bind, str) COL_##name,
    COLUMNS_LIST
#undef X
        COLUMN_COUNT
};

enum
{
    SIGNAL_PROJECTS_SELECTED,
    SIGNAL_COUNT
};

static gint project_browser_signals[SIGNAL_COUNT];

struct _DPSFGProjectBrowser
{
    GtkBox parent_instance;
};

struct _DPSFGProjectBrowserClass
{
    GtkWidgetClass parent_class;
};

G_DEFINE_TYPE(DPSFGProjectBrowser, dpsfg_project_browser, GTK_TYPE_BOX)

static void column_view_activate_cb(
    GtkColumnView* self, guint position, gpointer user_pointer)
{
    GtkSelectionModel* selection_model = gtk_column_view_get_model(self);
    GListModel*        model =
        gtk_multi_selection_get_model(GTK_MULTI_SELECTION(selection_model));
    GtkTreeListRow* row =
        gtk_tree_list_model_get_row(GTK_TREE_LIST_MODEL(model), position);

    gtk_tree_list_row_set_expanded(row, !gtk_tree_list_row_get_expanded(row));

    g_object_unref(row);
}

static void selection_changed_cb(
    GtkSelectionModel* self,
    guint              position_hint,
    guint              n_items,
    gpointer           user_data)
{
}

static void
setup_listitem_cb(GtkListItemFactory* factory, GtkListItem* list_item)
{
    GtkWidget* label = gtk_label_new("");
    gtk_list_item_set_child(list_item, label);
}

static void
bind_listitem_cb(GtkListItemFactory* factory, GtkListItem* list_item)
{
    GtkWidget* label;

    label = gtk_list_item_get_child(list_item);
    gtk_label_set_label(GTK_LABEL(label), "test");
}

static GtkWidget* create_project_list(void)
{
    GtkStringList*       model;
    GtkMultiSelection*   selection_model;
    GtkWidget*           column_view;
    GtkListItemFactory*  item_factory;
    GtkColumnViewColumn* column;
    const char*          strings[] = {"hello 1", "hey 2", NULL};

    model = gtk_string_list_new(strings);
    selection_model = gtk_multi_selection_new(G_LIST_MODEL(model));
    column_view = gtk_column_view_new(GTK_SELECTION_MODEL(selection_model));
    /*gtk_column_view_set_show_row_separators(GTK_COLUMN_VIEW(column_view),
     * TRUE);*/
    gtk_widget_set_vexpand(column_view, TRUE);

    item_factory = gtk_signal_list_item_factory_new();
    g_signal_connect(
        item_factory, "setup", G_CALLBACK(setup_listitem_cb), NULL);
    g_signal_connect(item_factory, "bind", G_CALLBACK(bind_listitem_cb), NULL);
    column = gtk_column_view_column_new("Name", item_factory);
    gtk_column_view_append_column(GTK_COLUMN_VIEW(column_view), column);
    g_object_unref(column);

#define X(name, setup, bind, str)                                              \
    item_factory = gtk_signal_list_item_factory_new();                         \
    g_signal_connect(                                                          \
        item_factory, "setup", G_CALLBACK(setup_##setup##_cb), NULL);          \
    g_signal_connect(                                                          \
        item_factory,                                                          \
        "bind",                                                                \
        G_CALLBACK(bind_##bind##_cb),                                          \
        (void*)(intptr_t)COL_##name);                                          \
    column = gtk_column_view_column_new(str, item_factory);                    \
    gtk_column_view_append_column(GTK_COLUMN_VIEW(column_view), column);       \
    g_object_unref(column);
    // COLUMNS_LIST
#undef X

    // g_signal_connect(
    //     column_view,
    //     "activate",
    //     G_CALLBACK(column_view_activate_cb),
    //     project_browser);
    // g_signal_connect(
    //     selection_model,
    //     "selection-changed",
    //     G_CALLBACK(selection_changed_cb),
    //     project_browser);

    return column_view;
}

static GtkWidget* create_top_widget(void)
{
    GtkWidget* search;
    GtkWidget* scroll;
    GtkWidget* vbox;
    GtkWidget* groups;
    GtkWidget* paned;
    GtkWidget* column_view;

    search = gtk_entry_new();
    gtk_entry_set_icon_from_icon_name(
        GTK_ENTRY(search), GTK_ENTRY_ICON_PRIMARY, "edit-find-symbolic");

    scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scroll), TRUE);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_AUTOMATIC,
        GTK_POLICY_AUTOMATIC);
    column_view = create_project_list();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), column_view);
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_vexpand(scroll, TRUE);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_append(GTK_BOX(vbox), search);
    gtk_box_append(GTK_BOX(vbox), scroll);

    groups = gtk_button_new();

    paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_paned_set_start_child(GTK_PANED(paned), groups);
    gtk_paned_set_end_child(GTK_PANED(paned), vbox);
    gtk_paned_set_resize_start_child(GTK_PANED(paned), FALSE);
    gtk_paned_set_resize_end_child(GTK_PANED(paned), TRUE);
    gtk_paned_set_position(GTK_PANED(paned), 120);

    return paned;
}

static void dpsfg_project_browser_init(DPSFGProjectBrowser* self)
{
    GtkWidget* paned = create_top_widget();
    gtk_box_append(GTK_BOX(self), paned);
}

static void dpsfg_project_browser_dispose(GObject* object)
{
    DPSFGProjectBrowser* self = DPSFG_PROJECT_BROWSER(object);
    untrack_mem(self);
    G_OBJECT_CLASS(dpsfg_project_browser_parent_class)->dispose(object);
}

static void dpsfg_project_browser_class_init(DPSFGProjectBrowserClass* class)
{
    GObjectClass* object_class = G_OBJECT_CLASS(class);
    object_class->dispose = dpsfg_project_browser_dispose;
    // gtk_widget_class_set_layout_manager_type(
    //     GTK_WIDGET_CLASS(class), GTK_TYPE_BIN_LAYOUT);

    project_browser_signals[SIGNAL_PROJECTS_SELECTED] = g_signal_new(
        "projects-selected",
        G_OBJECT_CLASS_TYPE(object_class),
        G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
        0,
        NULL,
        NULL,
        NULL,
        G_TYPE_NONE,
        2,
        G_TYPE_POINTER,
        G_TYPE_INT);
}

GtkWidget* dpsfg_project_browser_new(void)
{
    DPSFGProjectBrowser* project_browser =
        g_object_new(DPSFG_TYPE_PROJECT_BROWSER, NULL);
    track_mem(project_browser, sizeof *project_browser, "DPSFGProjectBrowser");
    return GTK_WIDGET(project_browser);
}
