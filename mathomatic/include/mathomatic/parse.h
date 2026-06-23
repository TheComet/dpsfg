#pragma once

struct token_type;

void str_tolower(char *cp);
void put_up_arrow(int cnt, char *cp);
int isvarchar(int ch);
int paren_increment(int ch);
int is_mathomatic_operator(int ch);
void binary_parenthesize(struct token_type *p1, int n, int i);
void handle_negate(struct token_type *equation, int *np);
void give_priority(struct token_type *equation, int *np);
char *parse_section(struct token_type *equation, int *np, char *cp, int allow_space);
char *parse_equation(int n, char *cp);
char *parse_expr(struct token_type *equation, int *np, char *cp, int allow_space);
char *parse_var(long *vp, char *cp);
void remove_trailing_spaces(char *cp);
void set_error_level(char *cp);
int var_is_const(long v, double *dp);
int subst_constants(struct token_type *equation, int *np);
int my_strlcpy(char *dest, char *src, int n);
