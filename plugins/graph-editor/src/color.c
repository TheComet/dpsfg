#include "graph-editor/color.h"

/* -------------------------------------------------------------------------- */
struct color rgb(uint8_t r, uint8_t g, uint8_t b)
{
    struct color color;
    color.r = r;
    color.g = g;
    color.b = b;
    return color;
}

/* -------------------------------------------------------------------------- */
struct color hex_rgb(uint32_t rgb)
{
    struct color color;
    color.r = (rgb >> 16) & 0xFF;
    color.g = (rgb >> 8) & 0xFF;
    color.b = (rgb >> 0) & 0xFF;
    return color;
}

/* -------------------------------------------------------------------------- */
GdkRGBA hex_gdk(uint32_t rgb)
{
    GdkRGBA color;
    color.red   = ((rgb >> 16) & 0xFF) / 255.0;
    color.green = ((rgb >> 8) & 0xFF) / 255.0;
    color.blue  = ((rgb >> 0) & 0xFF) / 255.0;
    color.alpha = 1.0;
    return color;
}

/* -------------------------------------------------------------------------- */
struct color gdk_to_color(GdkRGBA gdk)
{
    struct color color;
    color.r = gdk.red * 255;
    color.g = gdk.green * 255;
    color.b = gdk.blue * 255;
    return color;
}

/* -------------------------------------------------------------------------- */
struct color highlight_color(struct color color)
{
    (void)color;
    return rgb(204, 128, 0);
}
