#pragma once

struct dpsfg_plugin_interface;

struct plugin_lib
{
    void*                    handle;
    struct dpsfg_plugin_interface* i;
};

/*!
 * Scans the filesystem for plugins, and loads each one it finds. The callback
 * function can inspect the plugin's info structure (plugin->i->info) and
 * signal whether it wants to keep the plugin loaded or not by returning:
 *   1: Callback takes ownership and must call @see plugin_unload() later.
 *   0: The plugin will be unloaded again. Callback does not take ownership.
 *  -1: The plugin will be unloaded again. Callback does not take ownership.
 *      Additionally, iteration stops, and this function returns -1.
 * @param[in] path The path to search in. Typically this will be
 * "share/dpsfg/plugins"
 * @return If the callback returns -1 at any point, this function also returns
 * -1. If all calls return 0, then this function also returns 0. If any
 * callback returns 1, then this function returns 1 (OR of all results).
 */
int plugin_scan(
    const char* path,
    int (*on_plugin)(struct plugin_lib plugin, void* user),
    void* user);

int  plugin_load(struct plugin_lib* plugin, const char* filepath);
void plugin_unload(struct plugin_lib* plugin);
