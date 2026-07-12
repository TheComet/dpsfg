#include "csfg/util/str.h"
#include "csfg/util/strlist.h"
#include "csfg/util/tracker.h"
#include "csfg/util/vec.h"
#include "ui/db.h"
#include "ui/editable_label.h"
#include "ui/project_browser.h"

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
    struct strlist* columns;
    struct _DPSFGProjectBrowser* project_browser;
    int project_id;
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
    DPSFGProjectBrowser* project_browser,
    int project_id
#define X(str, caps, name) , const char* name
        COLUMNS_LIST
#undef X
)
{
    DPSFGProjectListItem* self =
        g_object_new(DPSFG_TYPE_PROJECT_LIST_ITEM, NULL);
    self->project_browser = project_browser;
    self->project_id      = project_id;
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
    self->scratch = NULL;
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
static DPSFGProjectList*
dpsfg_project_list_new(DPSFGProjectBrowser* project_browser)
{
    DPSFGProjectList* self = g_object_new(DPSFG_TYPE_PROJECT_LIST, NULL);
    self->scratch          = dpsfg_project_list_item_new(
        project_browser, 1, SCRATCH_PROJECT_NAME, "", "");
    return self;
}
static void
dpsfg_project_list_append(DPSFGProjectList* self, DPSFGProjectListItem* item)
{
    int scratch_entry = 1;
    DPSFGProjectListItem_vec_push(&self->items, item);
    g_list_model_items_changed(
        G_LIST_MODEL(self), vec_count(self->items) - 1 + scratch_entry, 0, 1);
}
static void
dpsfg_project_list_remove_by_id(DPSFGProjectList* self, int project_id)
{
    DPSFGProjectListItem** item;
    int i;
    int scratch_entry = 1;
    vec_enumerate (self->items, i, item)
        if ((*item)->project_id == project_id)
        {
            g_object_unref(*item);
            DPSFGProjectListItem_vec_erase(self->items, i);
            g_list_model_items_changed(
                G_LIST_MODEL(self), i + scratch_entry, 1, 0);
            break;
        }
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
    const struct db_interface* dbi;
    struct db* db;
    DPSFGProjectList* project_list;
    GtkSingleSelection* selection_model;
    int current_project_id;
    int block_outgoing_signals;
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

static gint signals[SIGNAL_COUNT];

G_DEFINE_TYPE(DPSFGProjectBrowser, dpsfg_project_browser, GTK_TYPE_BOX)

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
        self,
        id
#define X(str, caps, name) , name
            COLUMNS_LIST
#undef X
    );
    dpsfg_project_list_append(self->project_list, item);
    return 0;
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
    if (self->block_outgoing_signals)
        return;

    if (self->current_project_id != -1)
    {
        g_signal_emit(
            self,
            signals[SIGNAL_PROJECT_DESELECTED],
            0,
            self->current_project_id);
    }

    if (new_project_id != -1)
    {
        g_signal_emit(
            self, signals[SIGNAL_PROJECT_SELECTED], 0, new_project_id);
    }

    self->current_project_id = new_project_id;
}
static void items_changed_cb(
    GListModel* self,
    guint position,
    guint removed,
    guint added,
    gpointer user_data)
{
    DPSFGProjectBrowser* project_browser = user_data;
    (void)self, (void)position, (void)removed, (void)added;
    handle_list_or_selection_changed(project_browser);
}
static void selection_changed_cb(
    GtkSelectionModel* self,
    guint position_hint,
    guint n_items,
    gpointer user_data)
{
    DPSFGProjectBrowser* project_browser = user_data;
    (void)self, (void)position_hint, (void)n_items;
    handle_list_or_selection_changed(project_browser);
}
static void
editable_label_text_changed_cb(DPSFGEditableLabel* editable, gpointer user_data)
{
    GtkListItem* list_item               = user_data;
    DPSFGProjectListItem* item           = gtk_list_item_get_item(list_item);
    DPSFGProjectBrowser* project_browser = item->project_browser;
    const char* current_name       = strlist_cstr(item->columns, COLUMN_NAME);
    const char* new_name           = dpsfg_editable_label_get_text(editable);
    const struct db_interface* dbi = project_browser->dbi;
    struct db* db                  = project_browser->db;

    if (strcmp(current_name, new_name) == 0)
        return;
    if (strcmp(new_name, SCRATCH_PROJECT_NAME) == 0)
        return;
    if (!*new_name)
    {
        /* Restore the original text */
        dpsfg_editable_label_set_text(editable, current_name);
        return;
    }

    if (item->project_id == SCRATCH_PROJECT_ID)
    {
        int new_project_id, n_items;

        new_project_id = dbi->project.new(
            db, new_name, project_list_on_row, project_browser);
        if (new_project_id < 0)
        {
            /* The rename can fail if there is another project with the same
             * name already */
            struct str* str;
            int counter = 1;
            str_init(&str);
            do
            {
                str_fmt(&str, "%s (%d)", new_name, counter++);
                new_project_id = dbi->project.new(
                    db, str_cstr(str), project_list_on_row, project_browser);
            } while (new_project_id < 0);
            str_deinit(str);
        }

        /* Restore the original text on the "Scratch" project */
        dpsfg_editable_label_set_text(editable, SCRATCH_PROJECT_NAME);

        /* The "Scratch" project has probably not saved all of its data to the
         * db yet. Deselecting it will cause all data to be saved */
        g_signal_emit(
            project_browser,
            signals[SIGNAL_PROJECT_DESELECTED],
            0,
            SCRATCH_PROJECT_ID);

        /* Move the data from the "Scratch" project to the new project */
        dbi->graph_data.move(db, SCRATCH_PROJECT_ID, new_project_id);
        dbi->plugin_data.move(db, SCRATCH_PROJECT_ID, new_project_id);
        project_browser->current_project_id = new_project_id;

        /* Select the new project so the moved data gets loaded */
        g_signal_emit(
            project_browser,
            signals[SIGNAL_PROJECT_SELECTED],
            0,
            new_project_id);

        /* Update UI without triggering any signals */
        project_browser->block_outgoing_signals = 1;
        {
            n_items = g_list_model_get_n_items(
                G_LIST_MODEL(project_browser->project_list));
            gtk_single_selection_set_selected(
                project_browser->selection_model, n_items - 1);
        }
        project_browser->block_outgoing_signals = 0;

        return;
    }

    if (dbi->project.rename(db, item->project_id, new_name) == 0)
        strlist_set_cstr(&item->columns, COLUMN_NAME, new_name);
    else
    {
        /* The rename can fail if there is another project with the same name
         * already */
        struct str* str;
        int counter = 1;
        str_init(&str);
        do
            str_fmt(&str, "%s (%d)", new_name, counter++);
        while (dbi->project.rename(db, item->project_id, str_cstr(str)) != 0);
        dpsfg_editable_label_set_text(editable, str_cstr(str));
        strlist_set_cstr(&item->columns, COLUMN_NAME, str_cstr(str));
        str_deinit(str);
    }
}
static void setup_listitem_cb(
    GtkListItemFactory* factory, GtkListItem* list_item, gpointer user_data)
{
    GtkWidget* label;
    enum column col = (enum column)(uintptr_t)user_data;
    (void)factory;

    if (col != COLUMN_NAME)
        label = gtk_label_new("");
    else
    {
        label = dpsfg_editable_label_new();
        g_signal_connect(
            label,
            "text-changed",
            G_CALLBACK(editable_label_text_changed_cb),
            list_item);
    }
    gtk_list_item_set_child(list_item, label);
}
static void bind_listitem_cb(
    GtkListItemFactory* factory, GtkListItem* list_item, gpointer user_data)
{
    enum column col            = (enum column)(uintptr_t)user_data;
    GtkWidget* label           = gtk_list_item_get_child(list_item);
    DPSFGProjectListItem* item = gtk_list_item_get_item(list_item);
    const char* text           = strlist_cstr(item->columns, col);
    (void)factory;

    if (col == COLUMN_NAME)
        dpsfg_editable_label_set_text(DPSFG_EDITABLE_LABEL(label), text);
    else
        gtk_label_set_text(GTK_LABEL(label), text);
}
static gboolean
shortcut_delete_cb(GtkWidget* widget, GVariant* unused, gpointer user_data)
{
    DPSFGProjectBrowser* self      = user_data;
    const struct db_interface* dbi = self->dbi;
    struct db* db                  = self->db;
    int project_id                 = self->current_project_id;
    (void)widget, (void)unused;

    if (project_id != SCRATCH_PROJECT_ID)
    {
        self->current_project_id = -1;
        if (dbi->project.recycle(db, project_id) == 0)
            dpsfg_project_list_remove_by_id(self->project_list, project_id);
        return TRUE;
    }

    g_signal_emit(
        self, signals[SIGNAL_PROJECT_DESELECTED], 0, SCRATCH_PROJECT_ID);

    dbi->graph_data.delete(db, SCRATCH_PROJECT_ID);
    dbi->plugin_data.delete(db, SCRATCH_PROJECT_ID);

    g_signal_emit(
        self, signals[SIGNAL_PROJECT_SELECTED], 0, SCRATCH_PROJECT_ID);

    return TRUE;
}
static void setup_global_shortcuts(DPSFGProjectBrowser* self)
{
    GtkEventController* controller;
    GtkShortcutTrigger* trigger;
    GtkShortcutAction* action;
    GtkShortcut* shortcut;
    int i;

    struct
    {
        guint keyval;
        GdkModifierType modifiers;
        gboolean (*callback)(GtkWidget*, GVariant*, gpointer);
    } table[] = {
        /* clang-format off */
        {GDK_KEY_Delete, GDK_NO_MODIFIER_MASK, shortcut_delete_cb},
        /* clang-format on */
    };

    controller = gtk_shortcut_controller_new();
    gtk_shortcut_controller_set_scope(
        GTK_SHORTCUT_CONTROLLER(controller), GTK_SHORTCUT_SCOPE_LOCAL);
    gtk_widget_add_controller(GTK_WIDGET(self), controller);

    for (i = 0; i != G_N_ELEMENTS(table); ++i)
    {
        trigger  = gtk_keyval_trigger_new(table[i].keyval, table[i].modifiers);
        action   = gtk_callback_action_new(table[i].callback, self, NULL);
        shortcut = gtk_shortcut_new(trigger, action);
        gtk_shortcut_controller_add_shortcut(
            GTK_SHORTCUT_CONTROLLER(controller), shortcut);
    }
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
    self->project_list       = dpsfg_project_list_new(self);
    self->selection_model =
        gtk_single_selection_new(G_LIST_MODEL(self->project_list));
    self->block_outgoing_signals = 0;

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

    setup_global_shortcuts(self);

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

    signals[SIGNAL_PROJECT_DESELECTED] = g_signal_new(
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
    signals[SIGNAL_PROJECT_SELECTED] = g_signal_new(
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
GtkWidget*
dpsfg_project_browser_new(const struct db_interface* dbi, struct db* db)
{
    DPSFGProjectBrowser* self = g_object_new(DPSFG_TYPE_PROJECT_BROWSER, NULL);
    self->dbi                 = dbi;
    self->db                  = db;
    return GTK_WIDGET(self);
}
void dpsfg_project_browser_reload_from_db(DPSFGProjectBrowser* self)
{
    dpsfg_project_list_clear(self->project_list);
    self->dbi->project.list(self->db, project_list_on_row, self);
    handle_list_or_selection_changed(self);
}
