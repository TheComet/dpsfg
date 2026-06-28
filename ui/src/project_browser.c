#include "csfg/util/strlist.h"
#include "csfg/util/tracker.h"
#include "csfg/util/vec.h"
#include "dpsfg/db.h"
#include "dpsfg/project_browser.h"

#define COLUMNS_LIST                                                           \
    X("Name", NAME, name)                                                      \
    X("Date", DATE, date)                                                      \
    X("Time", TIME, time)

enum column
{
#define X(str, caps, name) COLUMN_##caps,
    COLUMNS_LIST
#undef X
        COLUMN_COUNT
};

/* -------------------------------------------------------------------------- */
/* Project List Item */
/* -------------------------------------------------------------------------- */
struct _DPSFGProjectListItem
{
    GObject parent_instance;
    int project_id;
    struct strlist* columns;
};

#define DPSFG_TYPE_PROJECT_LIST_ITEM (dpsfg_project_list_item_get_type())
G_DECLARE_FINAL_TYPE(
    DPSFGProjectListItem,
    dpsfg_project_list_item,
    DPSFG,
    PROJECT_LIST_ITEM,
    GObject)
G_DEFINE_TYPE(DPSFGProjectListItem, dpsfg_project_list_item, G_TYPE_OBJECT)

static void dpsfg_project_list_item_finalize(GObject* object)
{
    DPSFGProjectListItem* self = DPSFG_PROJECT_LIST_ITEM(object);
    strlist_deinit(self->columns);
    G_OBJECT_CLASS(dpsfg_project_list_item_parent_class)->finalize(object);
}
static DPSFGProjectListItem* dpsfg_project_list_item_new(
    int project_id
#define X(str, caps, name) , const char* name
        COLUMNS_LIST
#undef X
)
{
    DPSFGProjectListItem* self =
        g_object_new(DPSFG_TYPE_PROJECT_LIST_ITEM, NULL);
    self->project_id = project_id;
    strlist_init(&self->columns);
#define X(str, caps, name) strlist_add_cstr(&self->columns, name);
    COLUMNS_LIST
#undef X
    return self;
}
static void dpsfg_project_list_item_class_init(DPSFGProjectListItemClass* class)
{
    GObjectClass* object_class = G_OBJECT_CLASS(class);
    object_class->finalize     = dpsfg_project_list_item_finalize;
}
static void dpsfg_project_list_item_init(DPSFGProjectListItem* self)
{
    (void)self;
}

/* -------------------------------------------------------------------------- */
/* Project List */
/* -------------------------------------------------------------------------- */
VEC_DECLARE(DPSFGProjectListItem_vec, DPSFGProjectListItem*, 32)
VEC_DEFINE(DPSFGProjectListItem_vec, DPSFGProjectListItem*, 32)
struct _DPSFGProjectList
{
    GObject parent_instance;
    struct DPSFGProjectListItem_vec* items;
    DPSFGProjectListItem* scratch;
};

#define DPSFG_TYPE_PROJECT_LIST (dpsfg_project_list_get_type())
G_DECLARE_FINAL_TYPE(
    DPSFGProjectList, dpsfg_project_list, DPSFG, PROJECT_LIST, GObject)

static GType dpsfg_project_list_get_item_type(GListModel* list)
{
    (void)list;
    return DPSFG_TYPE_PROJECT_LIST_ITEM;
}
static guint dpsfg_project_list_get_n_items(GListModel* list)
{
    DPSFGProjectList* self = DPSFG_PROJECT_LIST(list);
    int scratch_entry      = 1;
    return vec_count(self->items) + scratch_entry;
}
static gpointer dpsfg_project_list_get_item(GListModel* list, guint position)
{
    DPSFGProjectList* self = DPSFG_PROJECT_LIST(list);
    if (position >= dpsfg_project_list_get_n_items(list))
        return NULL;
    if (position == 0)
        return g_object_ref(self->scratch);
    return g_object_ref(*vec_get(self->items, (int)position - 1));
}
static void dpsfg_project_list_model_init(GListModelInterface* iface)
{
    iface->get_item_type = dpsfg_project_list_get_item_type;
    iface->get_n_items   = dpsfg_project_list_get_n_items;
    iface->get_item      = dpsfg_project_list_get_item;
}
G_DEFINE_TYPE_WITH_CODE(
    DPSFGProjectList,
    dpsfg_project_list,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(G_TYPE_LIST_MODEL, dpsfg_project_list_model_init))
static void dpsfg_project_list_init(DPSFGProjectList* self)
{
    DPSFGProjectListItem_vec_init(&self->items);
    self->scratch = dpsfg_project_list_item_new(1, "Scratch", "", "");
}
static void dpsfg_project_list_dispose(GObject* object)
{
    DPSFGProjectListItem** item;
    DPSFGProjectList* self = DPSFG_PROJECT_LIST(object);
    vec_for_each (self->items, item)
        g_object_unref(*item);
    DPSFGProjectListItem_vec_deinit(self->items);
    g_object_unref(self->scratch);
    G_OBJECT_CLASS(dpsfg_project_list_parent_class)->dispose(object);
}
static void dpsfg_project_list_class_init(DPSFGProjectListClass* class)
{
    GObjectClass* object_class = G_OBJECT_CLASS(class);
    object_class->dispose      = dpsfg_project_list_dispose;
}
static DPSFGProjectList* dpsfg_project_list_new(void)
{
    return g_object_new(DPSFG_TYPE_PROJECT_LIST, NULL);
}
static void
dpsfg_project_list_append(DPSFGProjectList* self, DPSFGProjectListItem* item)
{
    DPSFGProjectListItem_vec_push(&self->items, item);
    g_list_model_items_changed(
        G_LIST_MODEL(self), vec_count(self->items), 0, 1);
}
static void dpsfg_project_list_clear(DPSFGProjectList* self)
{
    DPSFGProjectListItem** item;
    int items_removed = vec_count(self->items);
    vec_for_each (self->items, item)
        g_object_unref(*item);
    DPSFGProjectListItem_vec_clear(self->items);
    g_list_model_items_changed(G_LIST_MODEL(self), 0, items_removed, 0);
}

/* -------------------------------------------------------------------------- */
/* Project Browser */
/* -------------------------------------------------------------------------- */
struct _DPSFGProjectBrowser
{
    GtkBox parent_instance;
    DPSFGProjectList* project_list;
    GtkSingleSelection* selection_model;
    int current_project_id;
};
struct _DPSFGProjectBrowserClass
{
    GtkWidgetClass parent_class;
};
enum
{
    SIGNAL_PROJECT_DESELECTED,
    SIGNAL_PROJECT_SELECTED,
    SIGNAL_COUNT
};

static gint project_browser_signals[SIGNAL_COUNT];

G_DEFINE_TYPE(DPSFGProjectBrowser, dpsfg_project_browser, GTK_TYPE_BOX)

static void column_view_activate_cb(
    GtkColumnView* self, guint position, gpointer user_pointer)
{
    GtkSelectionModel* selection_model = gtk_column_view_get_model(self);
    GListModel* model =
        gtk_multi_selection_get_model(GTK_MULTI_SELECTION(selection_model));
    GtkTreeListRow* row =
        gtk_tree_list_model_get_row(GTK_TREE_LIST_MODEL(model), position);

    gtk_tree_list_row_set_expanded(row, !gtk_tree_list_row_get_expanded(row));

    g_object_unref(row);
    (void)user_pointer;
}
static void handle_list_or_selection_changed(DPSFGProjectBrowser* self)
{
    guint position   = gtk_single_selection_get_selected(self->selection_model);
    GListModel* list = gtk_single_selection_get_model(self->selection_model);
    GObject* item    = g_list_model_get_item(list, position);
    int new_project_id = item ? DPSFG_PROJECT_LIST_ITEM(item)->project_id : -1;
    if (item != NULL)
        g_object_unref(item);

    if (new_project_id == self->current_project_id)
        return;

    if (self->current_project_id != -1)
    {
        g_signal_emit(
            self,
            project_browser_signals[SIGNAL_PROJECT_DESELECTED],
            0,
            self->current_project_id);
    }

    if (new_project_id != -1)
    {
        g_signal_emit(
            self,
            project_browser_signals[SIGNAL_PROJECT_SELECTED],
            0,
            new_project_id);
    }

    self->current_project_id = new_project_id;
}
static void items_changed_cb(
    GListModel* self,
    guint position,
    guint removed,
    guint added,
    gpointer user_pointer)
{
    DPSFGProjectBrowser* project_browser = user_pointer;
    (void)self, (void)position, (void)removed, (void)added;
    handle_list_or_selection_changed(project_browser);
}
static void selection_changed_cb(
    GtkSelectionModel* self,
    guint position_hint,
    guint n_items,
    gpointer user_pointer)
{
    DPSFGProjectBrowser* project_browser = user_pointer;
    (void)self, (void)position_hint, (void)n_items;
    handle_list_or_selection_changed(project_browser);
}
static void setup_listitem_cb(
    GtkListItemFactory* factory, GtkListItem* list_item, gpointer user_data)
{
    GtkWidget* label = gtk_label_new(NULL);
    (void)factory, (void)user_data;

    gtk_list_item_set_child(list_item, label);
}
static void bind_listitem_cb(
    GtkListItemFactory* factory, GtkListItem* list_item, gpointer user_data)
{
    enum column col            = (enum column)(uintptr_t)user_data;
    GtkWidget* label           = gtk_list_item_get_child(list_item);
    DPSFGProjectListItem* item = gtk_list_item_get_item(list_item);
    (void)factory;

    gtk_label_set_label(GTK_LABEL(label), strlist_cstr(item->columns, col));
}
static void dpsfg_project_browser_init(DPSFGProjectBrowser* self)
{
    GtkListItemFactory* item_factory;
    GtkColumnViewColumn* column;
    GtkWidget* search;
    GtkWidget* scroll;
    GtkWidget* vbox;
    GtkWidget* column_view;

    track_mem(self, sizeof *self, "DPSFGProjectBrowser");

    self->current_project_id = -1;
    self->project_list       = dpsfg_project_list_new();
    self->selection_model =
        gtk_single_selection_new(G_LIST_MODEL(self->project_list));

    column_view =
        gtk_column_view_new(GTK_SELECTION_MODEL(self->selection_model));
    gtk_widget_set_hexpand(column_view, TRUE);
#define X(str, caps, name)                                                     \
    item_factory = gtk_signal_list_item_factory_new();                         \
    g_signal_connect(                                                          \
        item_factory,                                                          \
        "setup",                                                               \
        G_CALLBACK(setup_listitem_cb),                                         \
        (gpointer)(uintptr_t)COLUMN_##caps);                                   \
    g_signal_connect(                                                          \
        item_factory,                                                          \
        "bind",                                                                \
        G_CALLBACK(bind_listitem_cb),                                          \
        (gpointer)(uintptr_t)COLUMN_##caps);                                   \
    column = gtk_column_view_column_new(str, item_factory);                    \
    gtk_column_view_append_column(GTK_COLUMN_VIEW(column_view), column);       \
    g_object_unref(column);
    COLUMNS_LIST
#undef X

    search = gtk_entry_new();
    gtk_entry_set_icon_from_icon_name(
        GTK_ENTRY(search), GTK_ENTRY_ICON_PRIMARY, "edit-find-symbolic");

    scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scroll), TRUE);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_AUTOMATIC,
        GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), column_view);
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_vexpand(scroll, TRUE);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_append(GTK_BOX(vbox), search);
    gtk_box_append(GTK_BOX(vbox), scroll);

    gtk_box_append(GTK_BOX(self), vbox);

    g_signal_connect(
        self->selection_model,
        "selection-changed",
        G_CALLBACK(selection_changed_cb),
        self);
    g_signal_connect(
        self->project_list,
        "items-changed",
        G_CALLBACK(items_changed_cb),
        self);
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
    object_class->dispose      = dpsfg_project_browser_dispose;

    project_browser_signals[SIGNAL_PROJECT_DESELECTED] = g_signal_new(
        "project-deselected",
        G_OBJECT_CLASS_TYPE(object_class),
        G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
        0,
        NULL,
        NULL,
        NULL,
        G_TYPE_NONE,
        1,
        G_TYPE_INT);
    project_browser_signals[SIGNAL_PROJECT_SELECTED] = g_signal_new(
        "project-selected",
        G_OBJECT_CLASS_TYPE(object_class),
        G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
        0,
        NULL,
        NULL,
        NULL,
        G_TYPE_NONE,
        1,
        G_TYPE_INT);
}
GtkWidget* dpsfg_project_browser_new(void)
{
    return g_object_new(DPSFG_TYPE_PROJECT_BROWSER, NULL);
}
static int project_list_on_row(
    int id,
#define X(str, caps, name) const char *name,
    COLUMNS_LIST
#undef X
    void* user_data)
{
    DPSFGProjectListItem* item;
    DPSFGProjectBrowser* self = user_data;

    if (id == 1) /* scratch project is always at id=1 */
        return 0;

    item = dpsfg_project_list_item_new(
        id
#define X(str, caps, name) , name
            COLUMNS_LIST
#undef X
    );
    dpsfg_project_list_append(self->project_list, item);
    return 0;
}
void dpsfg_project_browser_reload_from_db(
    DPSFGProjectBrowser* self, const struct db_interface* dbi, struct db* db)
{
    dpsfg_project_list_clear(self->project_list);
    dbi->project.list_with_separate_date_time(db, project_list_on_row, self);
}
