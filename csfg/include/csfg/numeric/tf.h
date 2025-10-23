#pragma once

#include "csfg/numeric/poly.h"

struct csfg_tf
{
    struct csfg_cpoly* num;
    struct csfg_cpoly* den;

    struct csfg_rpoly* zeros;
    struct csfg_rpoly* poles;
};

static void csfg_tf_init(struct csfg_tf* tf) {
    csfg_cpoly_init(&tf->num);
    csfg_cpoly_init(&tf->den);
    csfg_rpoly_init(&tf->zeros);
    csfg_rpoly_init(&tf->poles);
}

static void csfg_tf_deinit(struct csfg_tf* tf) {
    csfg_rpoly_deinit(tf->poles);
    csfg_rpoly_deinit(tf->zeros);
    csfg_cpoly_deinit(tf->den);
    csfg_cpoly_deinit(tf->num);
}
