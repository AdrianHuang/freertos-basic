#ifndef SHELL_HISTORY_H
#define SHELL_HISTORY_H

int history_init(void);
int history_process_req(int, void *);

enum {
	HISTORY_ADD_CMD,
	HISTORY_COPY_CMD,
	HISTORY_SAVE_BUF_CMD,
	HISTORY_CHECK_ARROW
};
#endif
