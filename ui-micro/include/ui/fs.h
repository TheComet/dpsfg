#pragma once

struct str;

int fs_file_exists(const char* filepath);
int fs_dir_exists(const char* path);
int fs_make_dir(const char* path);
int fs_remove_file(const char* filepath);
int fs_list(
    const char* path,
    int (*on_entry)(const char* name, void* user),
    void* user);
