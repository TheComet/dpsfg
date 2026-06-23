#pragma once

struct token_type;

int differentiate(struct token_type *equation, int *np, long v);
int derivative_cmd(char *cp);
int extrema_cmd(char *cp);
int taylor_cmd(char *cp);
int limit_cmd(char *cp);
