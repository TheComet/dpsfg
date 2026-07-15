#pragma once

#include "csfg/util/vec.h"
#include "graph-editor/color.h"

struct csfg_graph;
struct serializer;
struct deserializer;

struct point
{
    int x, y;
};

VEC_DECLARE(point_vec, struct point, 16)

struct line
{
    struct point_vec* points;
    struct color color;
    int drag_begin_x, drag_begin_y;
    unsigned selected : 1;
};

VEC_DECLARE(line_vec, struct line, 16)

void line_init(struct line* line, struct color color);
void line_deinit(struct line* line);

int drawing_save(
    struct serializer** ser,
    const struct line_vec* drawing,
    const struct csfg_graph* g);
int drawing_save_line(struct serializer** ser, const struct line* line);
int drawing_load(
    struct deserializer* des,
    struct line_vec** drawing,
    const struct csfg_graph* g);

void drawing_clear(struct line_vec* drawing);
int try_select_line(const struct line_vec* drawing, int x, int y);
