/*
 *  File: sysstatus.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/07.
 *  Copyright 2018 All rights reserved.
 */

#include "sysstatus.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <memory.h>
#include "sysutils.h"
#include "stringutils.h"

int get_sys_mem_status(sys_mem_status *sms)
{
    printf("get_sys_mem_status ...\n");

    int rc = -1;

    char cmdBuffer[64];
    char resultBuffer[128];

    memset(cmdBuffer, 0, sizeof(cmdBuffer));
    memset(resultBuffer, 0, sizeof(resultBuffer));

    strcpy(cmdBuffer, "free | grep -i mem | awk '{print $2,$3,$4}'");

    printf("%s\n", cmdBuffer);

    cmd_system(cmdBuffer, resultBuffer, sizeof(resultBuffer));

    if (string_is_empty(resultBuffer)) {
        printf("get system memory status failed\n");
    }
    else {
        printf("system memory status: %s\n", resultBuffer);
        sscanf(resultBuffer,"%d %d %d", &sms->total, &sms->used, &sms->free);
        rc = 0;
    }

    return rc;
}

time_t get_sys_boot_time()
{
    printf("get_sys_boot_time\n");

    int rc = -1;

    char cmdBuffer[64];
    char resultBuffer[128];

    memset(cmdBuffer, 0, sizeof(cmdBuffer));
    memset(resultBuffer, 0, sizeof(resultBuffer));

    strcpy(cmdBuffer, "cat /proc/stat | grep btime | awk '{print $2}'");

    printf("%s\n", cmdBuffer);

    cmd_system(cmdBuffer, resultBuffer, sizeof(resultBuffer));

    if (string_is_empty(resultBuffer)) {
        printf("get sysstem boot time failed\n");
    }
    else {
        printf("get system boot time: %s\n", resultBuffer);
        if (string_is_number(resultBuffer)) {
            return atol(resultBuffer);
        }
        else {
            printf("get system boot time failed: not a number string\n");
        }

    }

    return rc;
}

int get_sys_load(sys_load_status *sls)
{
    printf("get_sys_load\n");

    char resultBuffer[128];
    memset(resultBuffer, 0, sizeof(resultBuffer));

    cmd_system("uptime", resultBuffer, sizeof(resultBuffer));

    if (string_is_empty(resultBuffer)) {
        printf("get system load failed\n");
        return -1;
    }
    else {
        printf("uptime: %s\n", resultBuffer);

        char *p = strrchr(resultBuffer, ':');
        if (!p || !(++p)) {
            return -1;
        }

        sscanf(p, "%f %f %f", &sls->sysLoadAverage1min, &sls->sysLoadAverage5min,&sls->sysLoadAverage15min);

        return 0;
    }
}
