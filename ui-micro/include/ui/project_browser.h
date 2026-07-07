#pragma once

struct mu_Context;

struct project_vec;
struct db_interface;
struct db;

struct project_browser
{
    struct project_vec* projects;
    int active_project_id;
};

void project_browser_init(struct project_browser* browser);
void project_browser_deinit(struct project_browser* browser);
int project_browser_load_from_db(
    struct project_browser* browser,
    const struct db_interface* dbi,
    struct db* db);
void project_browser_window(
    struct project_browser* browser, struct mu_Context* ctx);
