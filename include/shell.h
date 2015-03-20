#ifndef SHELL_H
#define SHELL_H

#define SHELL_CMD_LEN	128

int parse_command(char *str, char *argv[]);

typedef void cmdfunc(int, char *[]);

cmdfunc *do_command(const char *str);

#endif
