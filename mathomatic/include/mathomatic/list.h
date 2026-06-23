#pragma once

struct token_type;

/* A list of supported output languages for the code command: */
enum language_list {
	C = 1,      /* or C++ */
	JAVA = 2,
	PYTHON = 3
};

void reset_attr(void);
int set_color(int color);
void default_color(int set_no_color_flag);
int display_all_colors(void);
int list1_sub(int n, int export_flag);
int list_sub(int n);
void list_debug(int level, struct token_type *p1, int n1, struct token_type *p2, int n2);
char *var_name(long v);
int list_var(long v, int lang_code);
int list_proc(struct token_type *p1, int n, int export_flag);
char *list_equation(int n, int export_flag);
char *list_expression(struct token_type *p1, int n, int export_flag);
int list_string(struct token_type *p1, int n, char *string, int export_flag);
int list_string_sub(struct token_type *p1, int n, int outflag, char *string, int export_flag);
int int_expr(struct token_type *p1, int n);
int list_code_equation(int en, enum language_list language, int int_flag);
char *string_code_equation(int en, enum language_list language, int int_flag);
int list_code(struct token_type *equation, int *np, int outflag, char *string, enum language_list language, int int_flag);
char *flist_equation_string(int n);
int flist_equation(int n);
