cmake_minimum_required (VERSION 3.21)

project ("math-viewer"
    VERSION 0.0.1
    LANGUAGES C)

find_package (GTK4 REQUIRED COMPONENTS
    gtk glib cairo pango pangocairo harfbuzz gdk-pixbuf graphene)

include (DPSFGPlugin)
dpsfg_add_plugin ("math-viewer"
    SOURCES
        "src/plugin.c"
        "src/math_viewer.c"
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
        GTK4::gtk)

