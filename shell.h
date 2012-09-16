#ifndef _shell_h
#define _shell_h

#define MAX_COMMAND_ARGS 100

#include <apr_thread_proc.h>

typedef struct Shell
{
	const char *dir; // 目录
	const char *exe; // 要执行的命令

	apr_procattr_t *attr;// 进程属性，被封装
	apr_proc_t proc;// 进程，非封装
	apr_exit_why_e exit_why;// 枚举退出情况
	int exit_code;
	int needed_args;

	const char *args[MAX_COMMAND_ARGS];
} Shell;

int Shell_run(apr_pool_t *p, Shell *cmd);
int Shell_exec(Shell cmd, ...);

extern Shell CLEANUP_SH;
extern Shell GIT_SH;
extern Shell TAR_SH;
extern Shell CURL_SH;
extern Shell CONFIGURE_SH;
extern Shell MAKE_SH;
extern Shell INSTALL_SH;


#endif
