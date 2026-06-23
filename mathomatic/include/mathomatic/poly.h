#pragma once

struct token_type;

int poly_in_v_sub(struct token_type *p1, int n, long v, int allow_divides);
int poly_in_v(struct token_type *p1, int n, long v, int allow_divides);
int poly_factor(struct token_type *equation, int *np, int do_repeat);
int remove_factors(void);
int poly_gcd(struct token_type *larger, int llen, struct token_type *smaller, int slen, long v);
int poly2_gcd(struct token_type *larger, int llen, struct token_type *smaller, int slen, long v, int require_additive);
int is_integer_var(long v);
int is_integer_expr(struct token_type *p1, int n);
int mod_simp(struct token_type *equation, int *np);
int poly_gcd_simp(struct token_type *equation, int *np);
int div_remainder(struct token_type *equation, int *np, int poly_flag, int quick_flag);
int poly_div(struct token_type *d1, int len1, struct token_type *d2, int len2, long *vp);
int smart_div(struct token_type *d1, int len1, struct token_type *d2, int len2);
int basic_size(struct token_type *p1, int len);
int get_term(struct token_type *p1, int n1, int count, int *tp1, int *lentp1);
void term_value(double *dp, struct token_type *p1, int n1, int loc);
int find_greatest_power(struct token_type *p1, int n1, long *vp1, double *pp1, int *tp1, int *lentp1, int *dcodep);
