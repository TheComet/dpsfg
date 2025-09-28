#include "csfg/util/str.h"
#include "dpsfg/fs.h"

#if defined(_WIN32)
#else
#    include <dirent.h>
#    include <pwd.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#    include <unistd.h>
#endif

/* -------------------------------------------------------------------------- */
int fs_file_exists(const char* filepath)
{
#if defined(_WIN32)
    DWORD attr = GetFileAttributes(file_path);
    if (attr == INVALID_FILE_ATTRIBUTES)
        return 0;
    return !(attr & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    if (stat(filepath, &st))
        return 0;
    return S_ISREG(st.st_mode);
#endif
}

/* -------------------------------------------------------------------------- */
int fs_dir_exists(const char* filepath)
{
#if defined(_WIN32)
    DWORD attr = GetFileAttributes(filepath);
    if (attr == INVALID_FILE_ATTRIBUTES)
        return 0;
    return !!(attr & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    if (stat(filepath, &st))
        return 0;
    return S_ISDIR(st.st_mode);
#endif
}

/* -------------------------------------------------------------------------- */
int fs_make_dir(const char* path)
{
#if defined(_WIN32)
    if (CreateDirectory(path, NULL))
        return 0;
    return -1;
#else
    return mkdir(path, 0755) == 0;
#endif
}

/* -------------------------------------------------------------------------- */
int fs_remove_file(const char* path)
{
#if defined(_WIN32)
    if (DeleteFile(path))
        return 0;
    return -1;
#else
    return unlink(path);
#endif
}

/* -------------------------------------------------------------------------- */
int fs_list(
    const char* path, int (*on_entry)(const char* name, void* user), void* user)
{
#if defined(_WIN32)
    struct str*     correct_path;
    DWORD           dwError;
    WIN32_FIND_DATA ffd;
    int             ret = 0;
    HANDLE          hFind = INVALID_HANDLE_VALUE;

    str_init(&correct_path);
    if (str_set_cstr(&correct_path, path) != 0)
        goto str_set_failed;
    if (str_join_path_cstr(&correct_path, "*") != 0)
        goto first_file_failed;

    hFind = FindFirstFileA(str_cstr(correct_path), &ffd);
    if (hFind == INVALID_HANDLE_VALUE)
        goto first_file_failed;

    do
    {
        if (strcmp(fname, ".") == 0 || strcmp(fname, "..") == 0)
            continue;
        ret = on_entry(ffd.cFileName, user);
        if (ret != 0)
            goto out;
    } while (FindNextFile(hFind, &ffd) != 0);

    dwError = GetLastError();
    if (dwError != ERROR_NO_MORE_FILES)
        ret = -1;

out:
    FindClose(hFind);
first_file_failed:
    str_deinit(correct_path);
str_set_failed:
    return ret;
#else
    DIR*           dp;
    struct dirent* ep;
    int            ret = 0;

    dp = opendir(path);
    if (!dp)
        goto first_file_failed;

    while ((ep = readdir(dp)) != NULL)
    {
        struct strview fname = cstr_view(ep->d_name);
        if (strview_eq_cstr(fname, ".") || strview_eq_cstr(fname, ".."))
            continue;
        ret = on_entry(ep->d_name, user);
        if (ret != 0)
            goto out;
    }

out:
    closedir(dp);
first_file_failed:
    return ret;
#endif
}
