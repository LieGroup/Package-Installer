/*数据库，用来保存已安装过的文件的url，可以列出url、更新url、查找url*/
#include <unistd.h>
#include <apr_errno.h>
#include <apr_file_io.h>

#include "db.h"
#include "bstrlib.h"
#include "debug.h"

/*
bstring在bstrlib.h中的定义：
      typedef struct tagbstring * bstring;

tagbstring的定义：

 struct tagbstring {
	int mlen;
	int slen;
	unsigned char * data;
};
 */

static FILE *DB_open(const char *path, const char *mode)
{
	return fopen(path, mode);
}

static void DB_close(FILE *db)
{
	fclose(db);
}

/* 从数据库读取数据，并存入一个字符串中 */
static bstring DB_load()
{
	FILE *db = NULL;
	bstring data = NULL;

	db = DB_open(DB_FILE, "r");
	check(db, "Failed to open database: %s", DB_FILE);

	/*bstring bread (bNread readPtr, void * parm)
	 *
	 *  Use a finite buffer fread-like function readPtr to create a bstring 
	 *  filled with the entire contents of file-like source data in a roughly 
	 *  efficient way.
	 */
	data = bread((bNread)fread, db);
	check(data, "Failed to read from db file: %s", DB_FILE);

	DB_close(db);
	return data;

  error:
	if(db) DB_close(db);
	if(data) bdestroy(data);
	return NULL;
}

int DB_update(const char *url)
{
	if(DB_find(url))
	{
		log_info("Already recorded as installed: %s", url);
	}
	FILE *db = DB_open(DB_FILE, "a+");
	check(db, "Failed to open DB file: %s", DB_FILE);

	bstring line = bfromcstr(url);
	// bconchar将bstring和一个进行字符连接
	bconchar(line, '\n');
	int rc = fwrite(line->data, blength(line),1,db);
	check(rc == 1, "Failed to append to the db.");
	return 0;

  error:
	if(db) DB_close(db);
	return -1;
}

int DB_find(const char *url)
{
	bstring data = NULL;
	bstring line = bfromcstr(url);
	int res = -1;

	data = DB_load(DB_FILE);
	check(data, "Failed to load: %s", DB_FILE);
	// 从0开始，在data中找line
	if(binstr(data, 0, line) == BSTR_ERR)
	{
		res = 0;
	}
	else
	{
		res = 1;
	}

  error:
	if(data) bdestroy(data);
	if(line) bdestroy(line);

	return res;
}

int DB_init()
{

	apr_pool_t *p = NULL;// apr_pool_t被封装了，只能通过API对其进行操作
	apr_pool_initialize();
	apr_pool_create(&p, NULL); // 创建memory pool，返回apr_status_t类型的状态码

	// 确定DB_DIR是否存在、是否有写和执行权限，成功返回0，否则-1
	if(access(DB_DIR, W_OK | X_OK) == -1)
	{
		// 权限测试失败,可能是DB_DIR不存在，尝试创建
		apr_status_t rc = 
			apr_dir_make_recursive(DB_DIR,
								   APR_UREAD | APR_UWRITE | APR_UEXECUTE |
								   APR_GREAD | APR_GWRITE | APR_GEXECUTE, p);
		check(rc == APR_SUCCESS, "Failed to make database dir: %s", DB_DIR);
	}
	// 确定DB_FILE是否存在、是否有写权限
	if(access(DB_FILE, W_OK) == -1)
	{
		// 权限测试失败,可能是DB_FILE不存在，尝试创建
		FILE *db = DB_open(DB_FILE, "w");
		check(db, "Cannot open database: %s", DB_FILE);
		DB_close(db);
	}

	apr_pool_destroy(p);
	return 0;

  error:
	apr_pool_destroy(p);
	return -1;
}

int DB_list()
{
	bstring data = DB_load();
	check(data, "Failed to read load: %s", DB_FILE);
	// bdata(data)即data->data
	printf("%s", bdata(data));
	bdestroy(data);
	return 0;

  error:
	return -1;
}
