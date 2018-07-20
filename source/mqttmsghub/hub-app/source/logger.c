#include "logger.h"
#include <limits.h>
#include <string.h>
#include <unistd.h> 
#include "filepathutils.h"
#include "zlog.h"
#include "common.h"

#define LOG_CONGFILE_FILE "zlog.conf"

/*
 Init zlog
*/
int  Log_init(char *logCategory) 
{
    char cfgPath[_POSIX_PATH_MAX];
    int rc = -1;

    memset(cfgPath, 0 ,sizeof(cfgPath));

    local_path(cfgPath);
    strcat(cfgPath,"/");
    strcat(cfgPath, (char*)(LOG_CONGFILE_FILE));

    printf("Log conf: %s\n", cfgPath);

    if (access(cfgPath, F_OK) != 0) {
        printf("log config file doesn't exit\n");
        return FAILURE;
    }

    rc = dzlog_init(cfgPath,(char*)(logCategory));
    if (rc < 0) {
        printf("zlog initialzed failed %d\n",rc);
        return rc;
    }

    Log_info("--- zlog initialzed successully ---");

    return SUCCESS;
}

/*
 Deinit zlog
*/
void Log_deinit() 
{
    zlog_fini();
}