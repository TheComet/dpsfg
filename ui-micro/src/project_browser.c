#include "csfg/util/strlist.h"
#include "csfg/util/vec.h"
#include "microui.h"
#include "ui/db.h"
#include "ui/project_browser.h"

#define COLUMNS_LIST                                                           \
    X("Date", DATE, date)                                                      \
    X("Name", NAME, name)

enum column
{
#define X(str, caps, name) COLUMN_##caps,
    COLUMNS_LIST
#undef X
        COLUMN_COUNT
};

struct project
{
    struct strlist* columns;
    int id;
};

VEC_DECLARE(project_vec, struct project, 32)
VEC_DEFINE(project_vec, struct project, 32)

/* -------------------------------------------------------------------------- */
static int on_row(
    int id,
#define X(str, caps, name) const char *name,
    COLUMNS_LIST
#undef X
    void* user_data)
{
    struct project_vec** projects = user_data;
    struct project* project       = project_vec_emplace(projects);
    if (project == NULL)
        return -1;

    project->id = id;

    strlist_init(&project->columns);
#define X(str, caps, name)                                                     \
    if (strlist_add_cstr(&project->columns, name) != 0)                        \
    {                                                                          \
        project_vec_pop(*projects);                                            \
        return -1;                                                             \
    }
    COLUMNS_LIST
#undef X

    return 0;
}

/* -------------------------------------------------------------------------- */
static void clear_projects(struct project_browser* browser)
{
    struct project* project;
    vec_for_each (browser->projects, project)
        strlist_deinit(project->columns);
    project_vec_clear(browser->projects);
}

/* -------------------------------------------------------------------------- */
void project_browser_init(struct project_browser* browser)
{
    project_vec_init(&browser->projects);
    browser->active_project_id = -1;
}

/* -------------------------------------------------------------------------- */
void project_browser_deinit(struct project_browser* browser)
{
    clear_projects(browser);
    project_vec_deinit(browser->projects);
}

/* -------------------------------------------------------------------------- */
int project_browser_load_from_db(
    struct project_browser* browser,
    const struct db_interface* dbi,
    struct db* db)
{
    clear_projects(browser);
    return dbi->project.list(db, on_row, &browser->projects);
}

/* -------------------------------------------------------------------------- */
void project_browser_window(struct project_browser* browser, mu_Context* mu)
{
    int c;
    const struct project* project;
    int project_idx;
    int request_open_project_idx = -1;
    int widths[COLUMN_COUNT]     = {0};

    vec_for_each (browser->projects, project)
        for (c = 0; c != COLUMN_COUNT; ++c)
        {
            struct strview name;
            int width;
            if (c == COLUMN_NAME)
                continue;
            name  = strlist_view(project->columns, c);
            width = mu->text_width(
                mu->style->font, strview_data(name), strview_len(name));
            if (widths[c] < width + 10)
                widths[c] = width + 10;
        }
    widths[COLUMN_NAME] = -10;

    if (mu_begin_window_ex(
            mu, "Project Browser", mu_rect(10, 10, 200, 500), MU_OPT_NOCLOSE))
    {
        int width = -1;
        mu_layout_row(mu, 1, &width, 12+mu->text_height(mu->style->font));
        vec_enumerate (browser->projects, project_idx, project)
        {
            struct strview name = strlist_view(project->columns, COLUMN_NAME);
            if (mu_begin_clickable_panel(mu, strview_cstr(name)))
            {
                browser->active_project_id = project->id;
                request_open_project_idx   = project_idx;
            }
            mu_layout_row(mu, COLUMN_COUNT, widths, 0);
            for (c = 0; c != COLUMN_COUNT; ++c)
                mu_label(mu, strlist_cstr(project->columns, c));
            mu_end_clickable_panel(mu);
        }
        mu_end_window(mu);
    }

    if (request_open_project_idx > -1)
    {
        mu_Container* cnt;
        project = vec_get(browser->projects, request_open_project_idx);
        cnt = mu_get_container(mu, strlist_cstr(project->columns, COLUMN_NAME));
        if (cnt != NULL)
            cnt->open = 1;
    }

    vec_for_each (browser->projects, project)
        if (mu_begin_window(
                mu,
                strlist_cstr(project->columns, COLUMN_NAME),
                mu_rect(220, 10, 500, 500)))
        {
            mu_end_window(mu);
        }
}
