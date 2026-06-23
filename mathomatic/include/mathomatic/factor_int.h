#pragma once

struct token_type;

int factor_one(double value);
double multiply_out_unique(void);
int display_unique(void);
int is_prime(void);
int factor_int(struct token_type *equation, int *np);
int factor_int_equation(int n);
int list_factor(struct token_type *equation, int *np, int factor_flag);
int factor_constants(struct token_type *equation, int *np, int level_code);
