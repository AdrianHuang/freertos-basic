#include "shell.h"
#include <stddef.h>
#include <stdlib.h>
#include "clib.h"
#include <string.h>
#include "fio.h"
#include "filesystem.h"

#include "FreeRTOS.h"
#include "task.h"
#include "host.h"

typedef struct {
	const char *name;
	cmdfunc *fptr;
	const char *desc;
} cmdlist;

struct test_funcs {
	const char *name;
	int general_output;
	int (*test_func) (int);
};

void ls_command(int, char **);
void man_command(int, char **);
void cat_command(int, char **);
void ps_command(int, char **);
void host_command(int, char **);
void help_command(int, char **);
void host_command(int, char **);
void mmtest_command(int, char **);
void test_command(int, char **);
void _command(int, char **);

#define MKCL(n, d) {.name=#n, .fptr=n ## _command, .desc=d}

cmdlist cl[]={
	MKCL(ls, "List directory"),
	MKCL(man, "Show the manual of the command"),
	MKCL(cat, "Concatenate files and print on the stdout"),
	MKCL(ps, "Report a snapshot of the current processes"),
	MKCL(host, "Run command on host"),
	MKCL(mmtest, "heap memory allocation test"),
	MKCL(help, "help"),
	MKCL(test, "test new function"),
	MKCL(, ""),
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

int parse_command(char *str, char *argv[]){
	int b_quote=0, b_dbquote=0;
	int i;
	int count=0, p=0;
	for(i=0; str[i]; ++i){
		if(str[i]=='\'')
			++b_quote;
		if(str[i]=='"')
			++b_dbquote;
		if(str[i]==' '&&b_quote%2==0&&b_dbquote%2==0){
			str[i]='\0';
			argv[count++]=&str[p];
			p=i+1;
		}
	}
	/* last one */
	argv[count++]=&str[p];

	return count;
}

void ls_command(int n, char *argv[]){
    fio_printf(1,"\r\n"); 
    int dir;
    if(n == 0){
        dir = fs_opendir("");
    }else if(n == 1){
        dir = fs_opendir(argv[1]);
        //if(dir == )
    }else{
        fio_printf(1, "Too many argument!\r\n");
        return;
    }
(void)dir;   // Use dir
}

int filedump(const char *filename){
	char buf[128];

	int fd=fs_open(filename, 0, O_RDONLY);

	if( fd == -2 || fd == -1)
		return fd;

	fio_printf(1, "\r\n");

	int count;
	while((count=fio_read(fd, buf, sizeof(buf)))>0){
		fio_write(1, buf, count);
    }
	
    fio_printf(1, "\r");

	fio_close(fd);
	return 1;
}

void ps_command(int n, char *argv[]){
	signed char buf[1024];
	vTaskList(buf);
        fio_printf(1, "\n\rName          State   Priority  Stack  Num\n\r");
        fio_printf(1, "*******************************************\n\r");
	fio_printf(1, "%s\r\n", buf + 2);	
}

void cat_command(int n, char *argv[]){
	if(n==1){
		fio_printf(2, "\r\nUsage: cat <filename>\r\n");
		return;
	}

    int dump_status = filedump(argv[1]);
	if(dump_status == -1){
		fio_printf(2, "\r\n%s : no such file or directory.\r\n", argv[1]);
    }else if(dump_status == -2){
		fio_printf(2, "\r\nFile system not registered.\r\n", argv[1]);
    }
}

void man_command(int n, char *argv[]){
	if(n==1){
		fio_printf(2, "\r\nUsage: man <command>\r\n");
		return;
	}

	char buf[128]="/romfs/manual/";
	strcat(buf, argv[1]);

    int dump_status = filedump(buf);
	if(dump_status < 0)
		fio_printf(2, "\r\nManual not available.\r\n");
}

void host_command(int n, char *argv[]){
    int i, len = 0, rnt;
    char command[128] = {0};

    if(n>1){
        for(i = 1; i < n; i++) {
            memcpy(&command[len], argv[i], strlen(argv[i]));
            len += (strlen(argv[i]) + 1);
            command[len - 1] = ' ';
        }
        command[len - 1] = '\0';
        rnt=host_action(SYS_SYSTEM, command);
        fio_printf(1, "\r\nfinish with exit code %d.\r\n", rnt);
    } 
    else {
        fio_printf(2, "\r\nUsage: host 'command'\r\n");
    }
}

void help_command(int n,char *argv[]){
	int i;
	fio_printf(1, "\r\n");
	for(i = 0;i < sizeof(cl)/sizeof(cl[0]) - 1; ++i){
		fio_printf(1, "%s - %s\r\n", cl[i].name, cl[i].desc);
	}
}

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
	fio_printf(2, "Usage: test function_name number\r\n");
	fio_printf(2, "Example:\r\n");

	for(ptr=funcs_list;ptr->name;ptr++)
		fio_printf(2, "\ttest %s 5\r\n", ptr->name);

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

void test_command(int n, char *argv[])
{
	struct test_funcs *ptr;

	fio_printf(1, "\r\n");

	if (n < 3) {
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

void _command(int n, char *argv[]){
	(void)n; (void)argv;
	fio_printf(1, "\r\n");
}

cmdfunc *do_command(const char *cmd){

	int i;

	for(i=0; i<sizeof(cl)/sizeof(cl[0]); ++i){
		if(strcmp(cl[i].name, cmd)==0)
			return cl[i].fptr;
	}
	return NULL;	
}
