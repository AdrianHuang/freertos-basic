#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "fio.h"
#include "filesystem.h"
#include "clib.h"

static unsigned int new_task_num;
static xTaskHandle task_handle[32];

size_t xPortGetFreeHeapSize(void);

static void new_task(void *pvParameters)
{
	const portTickType ticks = 100 / portTICK_RATE_MS; // 100ms
	while (1) {
		vTaskDelay(ticks);
	}
}

void delete_command(int n, char *argv[])
{
	int task_num;

	if (n != 2) {
		fio_printf(2, "\r\nUsage: delete task_number\r\n");
		return;
	}

	task_num = atoi(argv[1]);

	vTaskDelete(task_handle[task_num]);

	fio_printf(1, "\r\nThe task 'New %d' is deleted!\r\n", task_num);
	return ;
}

void mem_command(int n, char *argv[])
{
	fio_printf(1, "\r\nRemaining Memory Size (Bytes): %d\r\n",
		  xPortGetFreeHeapSize());
}

void new_command(int n, char *argv[])
{
	char buf[configMAX_TASK_NAME_LEN];
	int status;

	fio_printf(1, "\r\n");

	while (1) {
		sprintf(buf, "New %d", new_task_num);
		status = xTaskCreate(new_task,
		            	     (signed portCHAR *) buf,
	        	    	     configMINIMAL_STACK_SIZE, NULL,
				     tskIDLE_PRIORITY + 1,
				     &task_handle[new_task_num]);

		if (status == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY) {
			fio_printf(1, "Could not create a new task!\r\n");
			break;
		}

		new_task_num++;

		fio_printf(1, "Crate a new task '%s'\r\n", buf);

		/*
		 * The command 'new' only creates a task and exit the loop.
		 * The command 'new -a' creates a number of tasks depending
		 * on the remaining memory size (heap size).
		 */
		if (n != 2 && strcmp(argv[1], "-a"))
			break;
	}
}


