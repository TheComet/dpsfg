#pragma once

/*!
 * @brief Appends a path to search for shared libraries.
 *
 * This is required for plugins that ship with their own shared library
 * dependencies. If the search path is not updated, then loading these plugins
 * will fail because the dynamic linker won't be able to find its dependencies.
 *
 * On Windows this calls SetDllDirectory()
 * On Linux this appends a path to LD_LIBRARY_PATH
 * @param[in] path Search path to add.
 * @return Returns 0 on success. Negative on error.
 */
int dynlib_add_path(const char* path);

/*!
 * @brief Loads a shared library and returns its handle.
 * @param[in] file_name UTF-8 path to file to load.
 * @return Returns a handle on success, NULL on failure.
 */
void* dynlib_open(const char* file_name);

/*!
 * @brief Unloads a previously loaded shared library.
 * @param[in] handle Handle returned by dynlib_open().
 */
void dynlib_close(void* handle);

/*!
 * @brief Looks up the address of the specified symbol and returns it.
 * @param[in] handle Handle returned by dynlib_open().
 * @param[in] name Symbol name
 * @return Returns the address to the symbol if successful or NULL on error.
 */
void* dynlib_symbol_addr(void* handle, const char* name);
