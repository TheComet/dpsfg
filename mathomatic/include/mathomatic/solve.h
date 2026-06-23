#pragma once

struct token_type;

int solve_espace(int want, int have);
int solve_sub(struct token_type *wantp, int wantn, struct token_type *leftp, int *leftnp, struct token_type *rightp, int *rightnp);
