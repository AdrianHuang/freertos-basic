#ifndef SHELL_H
#define SHELL_H

#define SHELL_CMD_LEN	128
#define SHELL_CMD_LEN_MASK (SHELL_CMD_LEN - 1)

int parse_command(char *str, char *argv[]);

typedef void cmdfunc(int, char *[]);

cmdfunc *do_command(const char *str);

#endif
