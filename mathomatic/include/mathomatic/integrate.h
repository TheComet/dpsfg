#pragma once

struct token_type;

void make_powers(struct token_type *equation, int *np, long v);
int int_dispatch(struct token_type *equation, int *np, long v, int (*func)(struct token_type *equation, int *np, int loc, int eloc, long v));
int integrate_cmd(char *cp);
int laplace_cmd(char *cp);
int nintegrate_cmd(char *cp);
