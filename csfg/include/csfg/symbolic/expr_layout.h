#pragma once

enum csfg_layout_type
{
    CSFG_LAYOUT_VALUE,
    CSFG_LAYOUT_BINOP,
    CSFG_LAYOUT_FRACTION,
    CSFG_LAYOUT_SUPERSCRIPT,
    CSFG_LAYOUT_SUBSCRIPT
};

struct csfg_layout_node
{
    int adv_x, height;
};
