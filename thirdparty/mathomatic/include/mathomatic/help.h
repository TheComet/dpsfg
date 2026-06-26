#pragma once

int parse(int n, char *cp);
int process_parse(int n, char *cp);
int process(char *cp);
int process_rv(char *cp);
int display_process(char *cp);
int shell_out(char *cp);
char *parse_var2(long *vp, char *cp);
int display_usage(char *pstr, int i);
int display_command(int i);
int display_repeat_command(void);
int read_examples(char **cpp);
void underline_title(int count);
int help_cmd(char *cp);
