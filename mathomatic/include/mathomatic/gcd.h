#pragma once

struct token_type;

double gcd(double d1, double d2);
double gcd_verified(double d1, double d2);
double my_round(double d1);
int f_to_fraction(double d, double *numeratorp, double *denominatorp);
int make_fractions(struct token_type *equation, int *np);
int make_simple_fractions(struct token_type *equation, int *np);
int make_mixed_fractions(struct token_type *equation, int *np);
