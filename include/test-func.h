#ifndef TEST_FUNC_H
#define TEST_FUNC_H

#define TEST_RX_QUEUE_LEN 1

struct test_func_args {
	int n;
	char **argv;
};

extern volatile xQueueHandle test_rx_queue;

int test_func_init(void);
#endif
