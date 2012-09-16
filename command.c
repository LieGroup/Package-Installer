#include <apr_uri.h>
#include <apr_fnmatch.h>
#include <unistd.h>

#include "command.h"
#include "debug.h"
#include "bstrlib.h"
#include "db.h"
#include "shell.h"

// 从一个depends文件中读取每一行，并安装其指示的软件
int Command_depends(apr_pool_t *p, const char *path) 
{
	FILE *in = NULL;
	bstring line = NULL;

	in = fopen(path, "r");
	check(in != NULL, "Failed to open downloaded depends: %s", path);

	// bgets调用fgetc()从in中读取字符，直到遇到'\n'，并存入line中
	for(line = bgets((bNgetc)fgetc, in, '\n');
		line != NULL;
		line = bgets((bNgetc)fgetc, in, '\n'))
	{
		btrimws(line); // trim掉头尾的空白字符
		log_info("Processing depends: %s", bdata(line));
		int rc = Command_install(p, bdata(line), NULL, NULL, NULL);
		check(rc == 0, "Failed to install: %s", bdata(line));
		bdestroy(line);
	}

	fclose(in);
	return 0;

  error:
	if(line) bdestroy(line);
	if(in) fclose(in);
	return -1;
}

int Command_fetch(apr_pool_t *p, const char *url, int fetch_only)
{
	apr_uri_t info = {.port = 0};// 端口设为0
	int rc = 0;
	const char *depends_file =NULL;
	apr_status_t rv = apr_uri_parse(p, url, &info); // 解析URL，放入&info

	check(rv == APR_SUCCESS, "Failed to parse URL: %s", url);

	// 尝试match "*.git"
	if(apr_fnmatch(GIT_PAT, info.path, 0) == APR_SUCCESS)
	{
		rc = Shell_exec(GIT_SH, "URL", url, NULL);
		check(rc == 0, "Git failed");
	}
	else if(apr_fnmatch(DEPEND_PAT, info.path, 0) == APR_SUCCESS)
	{
		check(!fetch_only, "No point in fetching a DEPENDS file.");
		if(info.scheme) // scheme表示协议类型，如http、ftp
		{
			depends_file = DEPENDS_PATH;
			rc = Shell_exec(CURL_SH, "URL", url, "TARGET",depends_file, NULL);
			check(rc == 0, "Curl failed");
		}
		else
		{
			depends_file = info.path;
		}

		// recursively process the devpkg list
		log_info("Building according to DEPENDS: %s", url);
		rv = Command_depends(p, depends_file);
		check(rv == 0, "Failed to process the DEPENDS: %s", url);

		// this indicates nothing needs to be done
		return 0;
	}
	else if(apr_fnmatch(TAR_GZ_PAT, info.path, 0) == APR_SUCCESS)
	{
		if(info.scheme)
		{
			rc = Shell_exec(CURL_SH, "URL", url, "TARGET", TAR_GZ_SRC, NULL);
			check(rc == 0, "Failed to curl source: %s", url);
							
		}

		rv = apr_dir_make_recursive(BUILD_DIR, 
									APR_UREAD | APR_UWRITE | APR_UEXECUTE, p);
		check(rv == APR_SUCCESS, "Failed to make directory %s", BUILD_DIR);
		rc = Shell_exec(TAR_SH, "FILE", TAR_GZ_SRC, NULL);
		check(rc == 0, "Failed to untar %s", TAR_GZ_SRC);

	}
	else if(apr_fnmatch(TAR_BZ2_PAT, info.path, 0) == APR_SUCCESS)
	{	
		if(info.scheme)
		{
			rc = Shell_exec(CURL_SH, "URL", url, "TARGET", TAR_BZ2_SRC, NULL);
			check(rc == 0, "Curl failed");
							
		}

		apr_status_t rc = apr_dir_make_recursive(BUILD_DIR, 
									APR_UREAD | APR_UWRITE | APR_UEXECUTE, p);
		check(rc == 0, "Failed to make directory %s", BUILD_DIR);
		rc = Shell_exec(TAR_SH, "FILE", TAR_GZ_SRC, NULL);
		check(rc == 0, "Failed to untar %s", TAR_GZ_SRC);
	}
	else
	{
		sentinel("Don't know how to handle %s", url);
	}

	// indicates that an install needs to actually run
	return 1;
  error:
	return -1;
}

int Command_build(apr_pool_t *p, const char *url, const char *configure_opts,
				  const char *make_opts, const char *install_opts)
{
	int rc  = 0;

	check(access(BUILD_DIR, X_OK | R_OK | W_OK) == 0,
		  "Build directory doesn't exist: %s", BUILD_DIR);

	// do an install
	if(access(CONFIG_SCRIPT, X_OK) == 0)
	{
		log_info("Has a configure script, running it.");
		rc = Shell_exec(CONFIGURE_SH, "OPTS", configure_opts,NULL);
		check(rc == 0, "Failed to configure");
	}

	rc = Shell_exec(MAKE_SH, "OPTS", make_opts, NULL);
	check(rc == 0, "Failed to build");

	rc = Shell_exec(INSTALL_SH, "TARGET", 
					 install_opts ? install_opts: "install", NULL);
	check(rc == 0, "Failed to install");

	rc = Shell_exec(CLEANUP_SH, NULL);
	check(rc == 0, "Failed to clean up after build");


	rc = DB_update(url);
	check(rc == 0, "Failed to add this package to the database");

	return 0;

  error:
	return -1;
}


int Command_install(apr_pool_t *p, const char *url, const char *configure_opts,
					const char *make_opts, const char *install_opts)
{
	int rc = 0;
	check(Shell_exec(CLEANUP_SH, NULL) == 0, 
		  "Failed to cleanup before install");

	// 检查该软件是否已经安装过
	rc = DB_find(url);
	check(rc != -1, "Error checking the install database");
	if(rc == 1)
	{
		log_info("Package %s already installed.", url);
		return 0;
	}

	// 未安装过，则获取之
	rc = Command_fetch(p, url, 0);
	if(rc == 1)	// 获取成功，开始安装
	{
		rc = Command_build(p, url, configure_opts, make_opts, install_opts);
		check(rc == 0, "Failed to build: %s", url);
	}
	else if(rc == 0) // 无需安装
	{
		log_info("Depends successfully installed: %s", url);
	}
	else
	{
		sentinel("Install failed: %s", url);	
	}

	Shell_exec(CLEANUP_SH,NULL);
	return 0;

  error:
	Shell_exec(CLEANUP_SH, NULL);
	return -1;


}
