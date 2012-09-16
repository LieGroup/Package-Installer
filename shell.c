#include "shell.h"
#include "debug.h"
#include <stdarg.h>

// 解析要执行的命令，获取命令和参数，调用Shell_run函数执行 
// "..."中的参数以<key, value>的形式成对出现
int Shell_exec(Shell template, ...)
{
	apr_pool_t *p = NULL;
	int rc = -1;
	apr_status_t rv = APR_SUCCESS;
	va_list argp;// 定义参数列表
	const char *key = NULL;
	const char *arg = NULL;
	int i = 0;
	int replaced_count = 0; // 统计替换了几个参数

	rv = apr_pool_create(&p, NULL);
	check(rv == APR_SUCCESS, "Failed to create pool");
	// 初始化参数列表argp的宏，template是调用函数时在argp前的最后一个参数
	va_start(argp, template);

	// va_arg()获取参数列表中的下一参数
	for(key = va_arg(argp, const char *);
		key != NULL;
		key = va_arg(argp, const char *))
	{
		arg = va_arg(argp, const char *);

		for(i = 0; template.args[i] != NULL; i++)
		{
			if(strcmp(template.args[i], key) == 0)
			{
				template.args[i] = arg;
				replaced_count++;
				break; // found it
			}
		}
	}

	//check(replaced_count >= template.needed_args, 
	//	  "You must give at least %d args", template.needed_args);
	rc = Shell_run(p, &template);
	apr_pool_destroy(p);
	va_end(argp);
	return rc;

  error:
	if(p)
	{
		apr_pool_destroy(p);
	}
	return rc;
}

int Shell_run(apr_pool_t *p, Shell *cmd)
{
	apr_procattr_t *attr;
	apr_status_t rv;
	apr_proc_t newproc;
	// 初始化attr
	rv = apr_procattr_create(&attr, p);
	check(rv == APR_SUCCESS, "Failed to create proc attr.");

	// 子进程启动时决定标准输入、输出、错误是否要连接到父进程的管道 
	// APR_NO_PIPE表示子进程共享父进程的标准输入、输出、错误
	rv = apr_procattr_io_set(attr, APR_NO_PIPE, APR_NO_PIPE, APR_NO_PIPE);
	check(rv == APR_SUCCESS, "Failed to set IO of command.");

	// 设置子进程执行的起始目录
	rv = apr_procattr_dir_set(attr, cmd->dir);
	check(rv == APR_SUCCESS, "Failed to set root to %s.", cmd->dir);

	// 设置子进程所执行的命令的类型，APR_PROGRAM_PATH表示执行PATH路径中的命令
	rv = apr_procattr_cmdtype_set(attr, APR_PROGRAM_PATH);
	check(rv == APR_SUCCESS, "Failed to set cmd type");

	/* 创建新进程，并在新进程终止前就返回：
	   执行cmd->exe程序
	   参数为cmd->args，第一个参数必须是程序名
	   新进程的环境变量
	   进程属性为attr
	   所用的pool为p
	*/
	rv = apr_proc_create(&newproc, cmd->exe, cmd->args, NULL, attr, p);
	check(rv == APR_SUCCESS, "Failed to run command.");

	// 等待子进程newproc结束，写入退出代码、原因和等待方式（是否等待子进程结束）
	rv = apr_proc_wait(&newproc, &cmd->exit_code, &cmd->exit_why, APR_WAIT);
	check(rv == APR_CHILD_DONE, "Failed to wait.");
	check(cmd->exit_code == 0, "%s exited badly.", cmd->exe);
	check(cmd->exit_why == APR_PROC_EXIT,"%s was killed or crashed", cmd->exe);

	return 0;

  error:
	return 1;
}

Shell CLEANUP_SH =
{
	.exe = "rm",
	.dir = "/tmp",
	.args = {"rm", "-rf", "/tmp/pkg-build", "/tmp/pkg-src.tar.gz",
			 "/tmp/pkg-src.tar.bz2", "/tmp/DEPENDS", NULL},
	.needed_args = 2
};

Shell GIT_SH =
{
	.exe = "git",
	.dir = "/tmp",
	.args = {"git", "clone", "URL", "pkg-build", NULL},
	.needed_args = 2
};

Shell TAR_SH =
{
	.exe = "tar",
	.dir = "/tmp/pkg-build",
	.args = {"tar", "-xzf", "FILE", "--strip-components", "1",NULL},
	.needed_args = 1
};


Shell CURL_SH =
{
	.exe = "curl",
	.dir = "/tmp",
	.args = {"curl", "-L", "-o", "TARGET", "URL",NULL},
	.needed_args = 2
};

Shell CONFIGURE_SH =
{
	.exe = "./configure",
	.dir = "/tmp/pkg-build",
	.args = {"configure", "OPTS", NULL},
	.needed_args = 2

};

Shell MAKE_SH =
{
	.exe = "make",
	.dir = "/tmp/pkg-build",
	.args = {"make", "OPTS", NULL},
	.needed_args = 2
};

Shell INSTALL_SH =
{
	.exe = "sudo",
	.dir = "/tmp/pkg-build",
	.args = {"sudo", "make", "TARGET",  NULL},
	.needed_args = 2
};


