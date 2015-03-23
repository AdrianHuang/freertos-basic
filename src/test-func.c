#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "clib.h"
#include "test-func.h"

#define TEST_FUNC_REQUIRE_ARGVS 3

volatile xQueueHandle test_rx_queue = NULL;

struct test_funcs {
	const char *name;
	int general_output;
	int (*test_func) (int);
};

/*
 * Definitions for test functions.
 */
#define MK_TEST_FUNC(n, g) {.name=#n, .general_output=g, .test_func=test_ ## n}

static int test_fib(int);
static int test_factorial(int);
static int test_prime(int);

static struct test_funcs funcs_list[] = {
	MK_TEST_FUNC(fib, 1),
	MK_TEST_FUNC(factorial, 1),
	MK_TEST_FUNC(prime, 0),
	{0}
};

static int test_fib(int n)
{
	int	fn = 1, fn1 = 0;

	if (n <= 0)
		return 0;

	while (n-- > 1) {
		fn = fn + fn1;
		fn1 = fn - fn1;
	}

	return fn;
}

static int test_factorial(int n)
{
	if (n <= 1)
		return 1;

	return n * test_factorial(n-1);
}

static int test_prime(int n)
{
	int i;

	for(i=2;i<n;i++) {
		if (!(n % i))
			break;
	}

	fio_printf(1, "%d is %sa prime number\r\n", n, i == n ? "" : "not ");

	/*
	 * return 1: is a prime number
	 * return 0: not a prime number
	 */
	return (i == n ? 1 : 0);
}

static void test_func_usage(void)
{
	struct test_funcs *ptr;
	fio_printf(1, "Usage: test function_name number\r\n");
	fio_printf(1, "Example:\r\n");

	for(ptr=funcs_list;ptr->name;ptr++)
		fio_printf(1, "\ttest %s 5\r\n", ptr->name);
}

static inline void test_invoke_func(struct test_funcs *p, char *arg)
{
	int result;
	int n = atoi(arg);

	result = p->test_func(n);

	if (p->general_output)
		fio_printf(1, "%s(%d) is %d\r\n", p->name, n, result);

	return;
}

static void do_test_command(int n, char *argv[])
{
	struct test_funcs *ptr;

	fio_printf(1, "\r\n");

	if (n < TEST_FUNC_REQUIRE_ARGVS) {
		test_func_usage();
		return ;
	}

	for(ptr=funcs_list;ptr->test_func;ptr++) {
		if (!strcmp(ptr->name, argv[1])) {
			test_invoke_func(ptr, argv[2]);
			break;
		}
	}

	if (!ptr->test_func)
		test_func_usage();

	return ;
}

static void test_func_task(void *pvParameters)
{
	const portTickType ticks = 100 / portTICK_RATE_MS; // 100ms
	int status;
	struct test_func_args args;

	while (1) {

		status = xQueueReceive(test_rx_queue, &args, ticks);

		if (status != pdPASS)
			continue;

		do_test_command(args.n, args.argv);
	}
}

int test_func_init(void)
{
	test_rx_queue = xQueueCreate(TEST_RX_QUEUE_LEN,
					sizeof(struct test_func_args));

	xTaskCreate(test_func_task,
	            (signed portCHAR *) "TEST",
	            512, NULL, tskIDLE_PRIORITY + 3, NULL);

	return 0;
}
