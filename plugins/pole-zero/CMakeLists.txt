cmake_minimum_required (VERSION 3.21)

project ("pole-zero"
    VERSION 0.0.1
    LANGUAGES C)

if (NOT DPSFG_MICROUI)
    find_package (GTK4 REQUIRED COMPONENTS
        gtk glib cairo pango pangocairo harfbuzz gdk-pixbuf graphene)
    set (GTK_LIBS
        GTK4::glib
        GTK4::cairo
        GTK4::pango
        GTK4::pangocairo
        GTK4::harfbuzz
        GTK4::gdk-pixbuf
        GTK4::graphene
        GTK4::gtk)
endif ()

include (DPSFGPlugin)
dpsfg_add_plugin ("pole-zero"
    SOURCES
        "src/plugin.c"
        "src/pole_zero_plot.c"
    INCLUDES
        "include"
        "${PROJECT_BINARY_DIR}/include"
    LIBS
        DPSFG::plugin
        DPSFG::csfg
        ${GTK_LIBS})
