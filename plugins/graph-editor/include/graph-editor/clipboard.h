#pragma once

struct _GdkClipboard;
struct graph_model;

void paste_from_clipboard(
    struct _GdkClipboard* clipboard, struct graph_model* model);
void copy_selection_to_clipboard(
    struct _GdkClipboard* clipboard, const struct graph_model* model);
