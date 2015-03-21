#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "fio.h"
#include "clib.h"
#include "filesystem.h"
#include "list_head.h"
#include "shell.h"
#include "shell-history.h"

#define HISTORY_FILE ".freertos_history"

#define HISTORY_MAX_ENTRY	10

#define HISTORY_ASSIGN_FUNC_PTR(h, name) \
	h->name = history_ ## name

struct history_entry {
	char cmd[SHELL_CMD_LEN];
	struct list_head list;
};

struct history_info {
	int cur_count; 	/* Currently avaialble entries in the list. */
	int max_count;	/* Maximum count in the list. */
	int prev_cmd_len;
	struct list_head *selected_list; /* in-memory cache. */
	struct list_head history_list;
};

struct history_func {
	int req;
	int (*func) (void *);
};

static void history_traverse_list(void);

static int history_get_from_file(void);
static int history_add_cmd(void *);
static int history_check_arrow(void *);
static int history_update_cmd(void *);

static struct history_func history_func_list[] = {
	{HISTORY_UPDATE_CMD, history_update_cmd},
	{HISTORY_CHECK_ARROW, history_check_arrow},
	{0}
};

static struct history_info *h_info;

static int history_get_from_file(void)
{
	char buffer[SHELL_CMD_LEN];
	char ch;
	int i = 0;
	struct history_entry *p;
	char fname[] = "/home/" USER_NAME "/" HISTORY_FILE;
	int fd = fs_open(fname, 0, O_RDONLY);

	/*
	 * FIXME: Reading the history file is not ready yet.
	 */
	if (fd < 0)
		return fd;

	while (fio_read(fd, &ch, 1) != 0) {
		if (ch != '\n') {
			buffer[i++] = ch;
			continue;
		} 
		
		buffer[i] = '\0';

		p = pvPortMalloc(sizeof(*p));
		if (!p) {
			fio_printf(2, "%s: malloc failed\r\n", __func__);
			return -1;
		}
		
		strncpy(p->cmd, buffer, strlen(buffer));
		list_add(&p->list, &h_info->history_list);
		h_info->cur_count++;

		i = 0;

		if(h_info->cur_count >= h_info->max_count)
			break;
	}

	fio_close(fd);
	return 0;
}

static int history_add_cmd(void *arg)
{
	struct history_entry *hep;
	char *cmd = (char *) arg;

	/* Reset the selected list. */
	h_info->selected_list = NULL;

	hep = list_first_entry(&h_info->history_list, 
			     	struct history_entry, list);

	/*
	 * Just return if the command is identical to the first command
	 * of the history list.
	 */
	if (!hep || !strncmp(hep->cmd, cmd, strlen(cmd)))
		return 0;

	/*
	 * If the history list is full, the oldest command will be
	 * replaced.
	 */
	if (h_info->cur_count >= h_info->max_count) {

		hep = list_last_entry(&h_info->history_list,
					struct history_entry, list);
		strcpy(hep->cmd, cmd);
		hep->cmd[strlen(cmd)] = '\0';

		list_move(&hep->list, &h_info->history_list);

		return 0;
	}

	/*
	 * The history list is not full: Allocate a new history
	 * and insert it into the history list.
	 */
	hep = pvPortMalloc(sizeof(*hep));
	if (!hep) {
		fio_printf(2, "%s: malloc failed\r\n", __func__);
		return 0;
	}

	strncpy(hep->cmd, cmd, strlen(cmd));
	list_add(&hep->list, &h_info->history_list);
	h_info->cur_count++;

	return 0;
}

static void history_traverse_list(void)
{
	struct list_head *p;
	struct history_entry *hep;
	int i = 0;

	list_for_each_prev(p, &h_info->history_list) {
		hep = list_entry(p, struct history_entry, list);
		fio_printf(1, "%d %s\r\n", ++i, hep->cmd);
	}

	return ;
}

static void history_display_cmd(struct list_head *p)
{
	char hint[] = USER_NAME "@" USER_NAME "-STM32:~$ ";
	struct history_entry *hep = list_entry(p, struct history_entry, list);
	int n = h_info->prev_cmd_len - strlen(hep->cmd);

	while (n-- > 0)
		fio_printf(1, "\b \b");

	h_info->prev_cmd_len = strlen(hep->cmd);

	fio_printf(1, "\r%s%s", hint, hep->cmd);

	/* Update the selected list. */
	h_info->selected_list = p;
}

static void history_clear_cmd(int n)
{
	while (n-- > 0)
		fio_printf(1, "\b \b");
}

static int history_check_arrow(void *arg)
{
	struct list_head *p;
	char ch = *((char *) arg);

	switch (ch) {
	case 'A': // up arrow 
		p = (h_info->selected_list ? h_info->selected_list :
			&h_info->history_list);

		if (list_empty(p) || (list_is_last(p, &h_info->history_list)))
			break;

		/* Display the next entry. */
		history_display_cmd(p->next);

		break;

	case 'B': // down arrow
		if (!h_info->selected_list)
			break;

		/* Reach the head of the history list. */
		if (h_info->selected_list == h_info->history_list.next) {
			history_clear_cmd(h_info->prev_cmd_len);
			h_info->prev_cmd_len = 0;
			h_info->selected_list = NULL;
			break;
		}

		/* Display the previous entry. */
		history_display_cmd(h_info->selected_list->prev);

		break;
	case 'C': // right arrow
	case 'D': // left arrow
	default:
		break;
	}
	
	return 0;
}

static int history_update_cmd(void *arg)
{
	struct history_entry *p;
	char *buf = (char *) arg;
	int len; 

	/*
	 * If the user did not hit any up or down key, just try to add
	 * the command to the history list.
	 */
	if (!h_info->selected_list)
		goto exit;

	p = list_entry(h_info->selected_list, struct history_entry, list);
	if (!p)
		goto exit;

	len = strlen(p->cmd);

	if (strlen(buf) > len)
		history_clear_cmd(strlen(buf) - len);

	strncpy(buf, p->cmd, len);
	buf[len] = '\0';

exit:
	/* Before returning, we need to add this command to history list. */
	history_add_cmd(buf);
	return 0;
}

int history_process_req(int req, void *arg)
{
	struct history_func *hfp;

	for(hfp=history_func_list;hfp->func;hfp++) {
		if (hfp->req == req) {
			return hfp->func(arg);
		}
	}

	return -1;
}

void history_command(int n, char *argv[])
{
	fio_printf(1, "\r\n");
	history_traverse_list();
}

int history_init(void)
{
	if (h_info)
		return 0;

	h_info = (struct history_info *) pvPortMalloc(sizeof(*h_info));
	if (!h_info) {
		fio_printf(2, "%s: malloc failed\r\n", __func__);
		return 0;
	}

	memset(h_info, 0, sizeof(*h_info)); 

	h_info->max_count = HISTORY_MAX_ENTRY;
	
	INIT_LIST_HEAD(&h_info->history_list);

	return history_get_from_file();
}
