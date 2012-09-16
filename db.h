/*数据库，用来保存已安装过的文件的url，可以列出url、更新url、查找url*/

#ifndef _db_h
#define _de_h

#define DB_FILE "/usr/local/.devpkg/db"
#define DB_DIR "/usr/local/.devpkg"

int DB_init();
int DB_list();
int DB_update(const char *url);
int DB_find(const char *url);

#endif
