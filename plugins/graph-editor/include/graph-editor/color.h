#pragma once

#include <gdk/gdk.h>
#include <stdint.h>

struct color
{
    uint8_t r, g, b;
};

struct color rgb(uint8_t r, uint8_t g, uint8_t b);
struct color hex_rgb(uint32_t rgb);
GdkRGBA hex_gdk(uint32_t rgb);
struct color gdk_to_color(GdkRGBA gdk);
struct color highlight_color(struct color color);
