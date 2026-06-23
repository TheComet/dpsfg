#pragma once

struct token_type;

int uf_tsimp(struct token_type *equation, int *np);
int uf_power(struct token_type *equation, int *np);
int uf_pplus(struct token_type *equation, int *np);
void uf_allpower(struct token_type *equation, int *np);
void uf_repeat(struct token_type *equation, int *np);
void uf_repeat_always(struct token_type *equation, int *np);
void uf_simp(struct token_type *equation, int *np);
void uf_simp_no_repeat(struct token_type *equation, int *np);
int ufactor(struct token_type *equation, int *np);
int uf_times(struct token_type *equation, int *np);
int sub_ufactor(struct token_type *equation, int *np, int ii);
int unsimp_power(struct token_type *equation, int *np);
void uf_neg_help(struct token_type *equation, int *np);
int patch_root_div(struct token_type *equation, int *np);
