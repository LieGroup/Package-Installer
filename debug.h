// 方便debug的头文件
#ifndef __dbg_h__
#define __dbg_h__


#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef NDEBUG
// 重新编译时删除已有的debug信息
#define debug(M, ...)
#else
/*
将debug("format string", arg1,arg2,...)转化为在stderr上的输出。__FILE__代表
当前正在使用的文件，__LINE__代表文件的当前行，__func__表示当前所在函数。
##__VA_ARGS__表示debug(M, ...)中的“...”任意参数
 */
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

// strerror输出更多的错误信息描述 
#define clean_errno() (errno == 0? "None": strerror(errno))

// log_err, log_warn, log_info的作用类似于debug（M, ...)
#define log_err(M, ...) fprintf(stderr, "[ERROR] (%s:%d:%s errno: %s)" M "\n", __FILE__, __LINE__, __func__,  clean_errno(), ##__VA_ARGS__)

#define log_warn(M, ...) fprintf(stderr, "[WARN] (%s:%d:%s errno: %s) " M "\n", __FILE__, __LINE__, __func__, clean_errno(), ##__VA_ARGS__)

#define log_info(M, ...) fprintf(stderr, "[INFO] (%s:%d:%s) " M "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__) 

#define check(A, M, ...) if(!(A)) { log_err(M, ##__VA_ARGS__); errno=0; goto error; } 

// 用来处理预期不应该发生的情况
#define sentinel(M, ...)  { log_err(M, ##__VA_ARGS__); errno=0; goto error; } 
// 检查指针的有效性
#define check_mem(A) check((A), "Out of memory.")

#define check_debug(A, M, ...) if(!(A)) { debug(M, ##__VA_ARGS__); errno=0; goto error; } 

#endif

