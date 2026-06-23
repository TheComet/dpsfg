#pragma once

struct token_type;

void group_proc(struct token_type *equation, int *np);
int fractions_and_group(struct token_type *equation, int *np);
int make_fractions_and_group(int n);
int super_factor(struct token_type *equation, int *np, int start_flag);
