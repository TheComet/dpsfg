#pragma once

struct _GdkClipboard;
struct graph_model;

void paste_from_clipboard(
    struct _GdkClipboard* clipboard,
    struct graph_model* model,
    int mouse_x,
    int mouse_y,
    void (*paste_complete_callback)(void*),
    void* user_data);
void copy_selection_to_clipboard(
    struct _GdkClipboard* clipboard, const struct graph_model* model);
