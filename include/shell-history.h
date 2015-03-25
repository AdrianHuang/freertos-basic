#ifndef SHELL_HISTORY_H
#define SHELL_HISTORY_H

#define SEND_HISTORY_REQ(r, a, t) 					\
do { 									\
	struct history_args arg = {0, r, (char **) a, (void *) t}; 	\
	xQueueSendToBack(history_rx_queue, &arg, 0); 			\
} while(0)

struct history_args {
	unsigned short n;
	unsigned short req;
	char **argv;
	void *ret;
};

enum {
	HISTORY_ADD_CMD = 1,
	HISTORY_COPY_CMD,
	HISTORY_SAVE_BUF_CMD,
	HISTORY_CHECK_ARROW
};

extern volatile xQueueHandle history_rx_queue;

int history_init(void);
#endif
