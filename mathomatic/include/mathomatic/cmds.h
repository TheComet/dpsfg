#pragma once

struct token_type;

#include <stdio.h>

int plot_cmd(char *cp);
int version_cmd(char *cp);
long max_memory_usage(void);
int show_status(FILE *ofp);
int version_report(void);
int solve_cmd(char *cp);
int sum_cmd(char *cp);
int product_cmd(char *cp);
int for_cmd(char *cp);
int optimize_cmd(char *cp);
int output_current_directory(FILE *ofp);
int fprintf_escaped(FILE *ofp, char *cp);
void output_options(FILE *ofp, int all_set_options);
int skip_no(char **cpp);
int save_set_options(char *cp);
int set_options(char *cp, int loading_startup_file);
int set_cmd(char *cp);
int echo_cmd(char *cp);
int pause_cmd(char *cp);
int copy_cmd(char *cp);
int real_cmd(char *cp);
int imaginary_cmd(char *cp);
int tally_cmd(char *cp);
int calculate_cmd(char *cp);
int clear_cmd(char *cp);
int compare_es(int i, int j);
int compare_cmd(char *cp);
int display_fraction(double value);
int divide_cmd(char *cp);
int eliminate_cmd(char *cp);
int display_cmd(char *cp);
int list_cmd(char *cp);
int code_cmd(char *cp);
int variables_cmd(char *cp);
int approximate_cmd(char *cp);
int replace_cmd(char *cp);
int simplify_cmd(char *cp);
int factor_cmd(char *cp);
int display_term_count(int en);
int unfactor_cmd(char *cp);
int div_loc_find(struct token_type *expression, int n);
int fraction_cmd(char *cp);
int quit_cmd(char *cp);
int read_cmd(char *cp);
int read_file(char *cp);
int read_sub(FILE *fp, char *filename);
int edit_cmd(char *cp);
int save_cmd(char *cp);
