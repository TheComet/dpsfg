#pragma once

struct csfg_expr_pool;

int csfg_expr_opt_fold_constants(struct csfg_expr_pool** pool, int* root);
