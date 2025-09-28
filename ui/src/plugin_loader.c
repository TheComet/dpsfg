#include "csfg/util/log.h"
#include "csfg/util/str.h"
#include "csfg/util/strview.h"
#include "dpsfg/dynlib.h"
#include "dpsfg/fs.h"
#include "dpsfg/plugin_loader.h"

struct fs_list_ctx
{
    const char*    scanpath;
    struct str*    filepath;
    struct strview subdir;
    int (*on_plugin)(struct plugin_lib plugin, void* user);
    void* on_plugin_user;
};

/* -------------------------------------------------------------------------- */
static int on_filename(const char* name, void* user)
{
    struct plugin_lib   plugin;
    int                 ret = 0; /* Try to continue by default */
    struct fs_list_ctx* ctx = user;
    struct strview      fname = cstr_view(name);

    /* Only consider files that have the same name as their parent directy */
    if (!strview_eq(strview_remove_file_ext(fname), ctx->subdir))
        return 0;

    /* PDB files on Windows */
#if defined(_DEBUG)
    if (cstr_ends_with(fname, ".pdb"))
        return 0;
#endif

    /* Join path to shared lib and try to load it. */
    if (str_join_path(&ctx->filepath, fname) != 0)
        return -1;

    if (plugin_load(&plugin, str_cstr(ctx->filepath)) != 0)
    {
        log_err("! Failed to load plugin %s\n", str_cstr(ctx->filepath));
        goto load_plugin_failed;
    }
    log_dbg("+ Found plugin %s\n", str_cstr(ctx->filepath));

    /*
     * If the callback returns positive it means they want to use the plugin.
     * Ownership of the library handle is transferred to them. Stop iterating.
     */
    ret = ctx->on_plugin(plugin, ctx->on_plugin_user);
    if (ret <= 0)
        plugin_unload(&plugin);

load_plugin_failed:
    str_dirname(ctx->filepath);
    return ret;
}

/* -------------------------------------------------------------------------- */
static int on_subdir(const char* subdir, void* user)
{
    struct fs_list_ctx* ctx = user;
    ctx->subdir = cstr_view(subdir);

    /*
     * Search in each subdirectory for a shared library matching the
     * directory's name. For example, if there is a directory "myplugin/",
     * then inside it there should be a "myplugin/myplugin.so/dll" file.
     */
    if (str_set_cstr(&ctx->filepath, ctx->scanpath) != 0)
        return -1;
    if (str_join_path(&ctx->filepath, ctx->subdir) != 0)
        return -1;

    /*
     * It is necessary to add the plugin directory to the DLL/library search
     * path. Otherwise the plugin won't be able to load its dependencies, if
     * it has any.
     */
    if (dynlib_add_path(str_cstr(ctx->filepath)) != 0)
        return 0; /* Try to continue */

    return fs_list(str_cstr(ctx->filepath), on_filename, ctx);
}

/* -------------------------------------------------------------------------- */
int plugin_scan(
    const char* path,
    int (*on_plugin)(struct plugin_lib plugin, void* user),
    void* user)
{
    int                ret;
    struct fs_list_ctx ctx;

    ctx.scanpath = path;
    str_init(&ctx.filepath);
    ctx.on_plugin = on_plugin;
    ctx.on_plugin_user = user;

    /* Scan for all folders/files in plugin directory */
    log_dbg("Scanning for plugins in %s\n", path);
    ret = fs_list(path, on_subdir, &ctx);

    str_deinit(ctx.filepath);
    return ret;
}

/* -------------------------------------------------------------------------- */
int plugin_load(struct plugin_lib* plugin, const char* filepath)
{
    plugin->handle = dynlib_open(filepath);
    if (plugin->handle == NULL)
        return log_err("Failed to load plugin %s\n", filepath);

    plugin->i = dynlib_symbol_addr(plugin->handle, "dpsfg_plugin");
    if (plugin->i == NULL)
    {
        dynlib_close(plugin->handle);
        return log_err(
            "'dpsfg_plugin' symbol not found in plugin %s\n", filepath);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
void plugin_unload(struct plugin_lib* plugin)
{
    dynlib_close(plugin->handle);
}
