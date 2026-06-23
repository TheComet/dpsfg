#pragma once

struct token_type;

int factor_divide(struct token_type *equation, int *np, long v, double d);
int subtract_itself(struct token_type *equation, int *np);
int factor_plus(struct token_type *equation, int *np, long v, double d);
int factor_times(struct token_type *equation, int *np);
int factor_power(struct token_type *equation, int *np);
