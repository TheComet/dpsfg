#include "csfg/io/deserialize.h"
#include "csfg/io/serialize.h"
#include "graph-editor/constants.h"
#include "graph-editor/drawing.h"

VEC_DEFINE(point_vec, struct point, 16)
VEC_DEFINE(line_vec, struct line, 16)

/* -------------------------------------------------------------------------- */
void line_init(struct line* line, struct color color)
{
    point_vec_init(&line->points);
    line->color        = color;
    line->drag_begin_x = 0;
    line->drag_begin_y = 0;
    line->selected     = 0;
}

/* -------------------------------------------------------------------------- */
void line_deinit(struct line* line)
{
    point_vec_deinit(line->points);
}

/* -------------------------------------------------------------------------- */
int drawing_save_line(struct serializer** ser, const struct line* line)
{
    const struct point* point;
    int err = 0;

    err += serialize_u8(ser, line->color.r);
    err += serialize_u8(ser, line->color.g);
    err += serialize_u8(ser, line->color.b);

    err += serialize_lu16(ser, vec_count(line->points));
    vec_for_each (line->points, point)
    {
        err += serialize_li16(ser, point->x);
        err += serialize_li16(ser, point->y);
    }

    return err;
}

/* -------------------------------------------------------------------------- */
int drawing_save(
    struct serializer** ser,
    const struct line_vec* drawing,
    const struct csfg_graph* g)
{
    const struct line* line;
    int err = 0;

    if (g == NULL)
        return 0;

    err += serialize_lu16(ser, vec_count(drawing));
    vec_for_each (drawing, line)
        err += drawing_save_line(ser, line);

    return err;
}

/* -------------------------------------------------------------------------- */
int drawing_load(
    struct deserializer* des,
    struct line_vec** drawing,
    const struct csfg_graph* g)
{
    int line_count, point_count;
    struct line* line;
    struct point* point;
    struct color color;

    if (g == NULL)
        return 0;

    drawing_clear(*drawing);

    line_count = deserialize_lu16(des);
    while (line_count-- > 0)
    {
        color.r = deserialize_u8(des);
        color.g = deserialize_u8(des);
        color.b = deserialize_u8(des);
        line    = line_vec_emplace(drawing);
        if (line == NULL)
            return -1;
        line_init(line, color);

        point_count = deserialize_lu16(des);
        while (point_count-- > 0)
        {
            point = point_vec_emplace(&line->points);
            if (point == NULL)
                return -1;
            point->x = deserialize_li16(des);
            point->y = deserialize_li16(des);
        }
    }

    return deserializer_err(des);
}

/* -------------------------------------------------------------------------- */
void drawing_clear(struct line_vec* drawing)
{
    while (vec_count(drawing))
        point_vec_deinit(line_vec_pop(drawing)->points);
}

/* -------------------------------------------------------------------------- */
int try_select_line(const struct line_vec* drawing, int x, int y)
{
    int line_idx;
    const struct line* line;
    const struct point* point;
    vec_enumerate (drawing, line_idx, line)
        vec_for_each (line->points, point)
            if (x <= point->x + GRID && x >= point->x - GRID &&
                y <= point->y + GRID && y >= point->y - GRID)
            {
                return line_idx;
            }
    return -1;
}
