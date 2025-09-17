#include "csfg/config.h"
#include "csfg/util/cli_colors.h"
#include "csfg/util/log.h"
#include "ui/args.h"
#include <string.h>

#define SECTION        COL_B_WHITE
#define TEXT           COL_B_WHITE
#define ARG1           COL_B_GREEN
#define ARG2           COL_B_YELLOW
#define RED            COL_B_RED
#define COMMENT        COL_B_CYAN
#define URL            COL_B_WHITE
#define VERSION_TEXT   COL_B_WHITE
#define VERSION_NUMBER COL_B_CYAN
#define RESET          COL_RESET

static int print_help(const char* prog_name)
{
    /* clang-format off */
    log_raw(SECTION "Usage:\n" RESET "  %s [" ARG2 "options" RESET "]\n\n", prog_name);

    log_raw(
        SECTION "Available options:\n" RESET
        "  " ARG2 "-h" RESET "," ARG1 " --help  " RESET "          Print this help text.\n");

#if defined(CSFG_TESTS)
    log_raw("     " ARG1 " --tests " RESET "          Run unit tests.\n");
#endif

    /* Disabled options */
#if !defined(CSFG_TESTS)
    log_raw(
        SECTION "\nDisabled options:\n" RESET);
#endif
#if !defined(CSFG_TESTS)
    log_raw("     " RED  " --tests " RESET "          (Recompile with -DCSFG_TESTS=ON)\n");
#endif
    /* clang-format on */

    return 1;
}

/* ------------------------------------------------------------------------- */
int args_parse(struct args* a, int argc, char* argv[])
{
    int  i;
    char tests_flag = 0;

    /* Set defaults */
    a->tests = 0;

    for (i = 1; i < argc; ++i)
    {
        if (argv[i][0] == '-')
        {
            if (argv[i][1] == '-')
            {
                const char* arg = &argv[i][2];
                if (strcmp(arg, "help") == 0)
                    return print_help(argv[0]);
#if defined(CSFG_TESTS)
                else if (strcmp(arg, "tests") == 0)
                    tests_flag = 1;
#endif
                else if (strcmp(argv[i], "--") == 0)
                    break;
                else if (tests_flag == 0)
                {
                    log_err("Unknown option \"%s\"\n", argv[i]);
                    return -1;
                }
            }
            else
            {
                const char* p;
                for (p = &argv[i][1]; *p; ++p)
                {
                    if (*p == 'h')
                        return print_help(argv[0]);
                    else
                    {
                        log_err("Unknown option \"-%c\"\n", *p);
                        return -1;
                    }
                }

                if (!argv[i][1])
                {
                    log_err("Unknown option \"%s\"\n", argv[i]);
                    return -1;
                }
            }
        }
        else
        {
            log_err("Invalid option \"%s\"\n", argv[i]);
            return -1;
        }
    }

    if (0)
    {
    }
#if defined(CSFG_TESTS)
    else if (tests_flag)
        a->tests = 1;
#endif

    return 0;
}
