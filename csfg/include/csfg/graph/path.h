#pragma once

#include "csfg/util/vec.h"

VEC_DECLARE(csfg_path_vec, int, 16)

struct csfg_path
{
    const int* edge_idxs;
};

static struct csfg_path csfg_paths_begin(const struct csfg_path_vec* paths)
{
    struct csfg_path path;
    path.edge_idxs = vec_begin(paths);
    return path;
}

static struct csfg_path csfg_paths_next(struct csfg_path path)
{
    while (*path.edge_idxs != -1)
        path.edge_idxs++;
    path.edge_idxs++;
    return path;
}

static int csfg_paths_count(const struct csfg_path_vec* paths)
{
    int        count = 0;
    const int* edge_idx;
    vec_for_each (paths, edge_idx)
        if (*edge_idx == -1)
            count++;
    return count;
}

static struct csfg_path
csfg_paths_get(const struct csfg_path_vec* paths, int idx)
{
    struct csfg_path path = csfg_paths_begin(paths);
    while (idx--)
        path = csfg_paths_next(path);
    return path;
}

#define csfg_paths_for_each(paths, path)                                       \
    for (path = csfg_paths_begin(paths); path.edge_idxs != vec_end(paths);     \
         path = csfg_paths_next(path))
#define csfg_paths_enumerate(paths, idx, path)                                 \
    for (path = csfg_paths_begin(paths), idx = 0;                              \
         path.edge_idxs != vec_end(paths);                                     \
         path = csfg_paths_next(path), ++idx)
