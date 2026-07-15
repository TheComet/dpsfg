cmake_minimum_required (VERSION 3.21)

project ("graph-editor"
    VERSION 0.0.1
    LANGUAGES C)

find_package (GTK4 REQUIRED COMPONENTS
    gtk glib cairo pango pangocairo harfbuzz gdk-pixbuf graphene librsvg)

gtk_compile_resource (
    "res/graph-editor.gresource.xml"
    "src/graph-editor.gresource.c")

include (DPSFGPlugin)
dpsfg_add_plugin ("graph-editor"
    SOURCES
        "${PROJECT_BINARY_DIR}/src/graph-editor.gresource.c"
        "res/graph-editor.gresource.xml"
        "src/attr.c"
        "src/clipboard.c"
        "src/color.c"
        "src/color_picker.c"
        "src/draw.c"
        "src/drawing.c"
        "src/geometry.c"
        "src/graph_editor.c"
        "src/graph_helpers.c"
        "src/graph_model.c"
        "src/undo.c"
        "src/plugin.c"
    INCLUDES
        "include"
        "${PROJECT_BINARY_DIR}/include"
    LIBS
        DPSFG::plugin
        DPSFG::csfg
        GTK4::glib
        GTK4::cairo
        GTK4::pango
        GTK4::pangocairo
        GTK4::harfbuzz
        GTK4::gdk-pixbuf
        GTK4::graphene
        GTK4::gtk
        GTK4::librsvg)
