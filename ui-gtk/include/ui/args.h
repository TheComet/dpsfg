#pragma once

struct args
{
    unsigned tests : 1;
};

int args_parse(struct args* a, int argc, char* argv[]);
