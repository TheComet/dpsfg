#pragma once

struct backtrace_vec;

int backtrace_init(void);
void backtrace_deinit(void);
void backtrace_vec_deinit(struct backtrace_vec* bt);

int backtrace_generate(struct backtrace_vec** bt);
void backtrace_log_vec(const struct backtrace_vec* bt);
void backtrace_log(void);
