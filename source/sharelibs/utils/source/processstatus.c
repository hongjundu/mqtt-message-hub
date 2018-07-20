/*
 *  File: processstatus.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#include "processstatus.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <memory.h>
#include "sysutils.h"
#include "stringutils.h"

pid_t get_process_id(const char *procesName)
{
    printf("get_process_id: %s\n", procesName);

    char cmdBuffer[128];
    char resultBuffer[64];

    memset(cmdBuffer, 0, sizeof(cmdBuffer));
    memset(resultBuffer, 0, sizeof(resultBuffer));

    sprintf(cmdBuffer, "ps aux | grep -i \"%s\" | grep -v \"grep\" | grep -v \"PPID\" | awk '{print $2}'",procesName);

    printf("Executing... %s\n",cmdBuffer);

    cmd_system(cmdBuffer, resultBuffer, sizeof(resultBuffer));

    printf("Result: %s\n", resultBuffer);

    if (string_is_empty(resultBuffer)) {
        return 0;
    }

    return atoi(resultBuffer);
}

int get_process_status(pid_t pid, process_status *ps)
{
    printf("get_process_status %d\n...", pid);

    int rc = -1;

    if (pid <= 0) {
        return rc;
    }

    char cmdBuffer[64];
    char resultBuffer[64];

    memset(cmdBuffer, 0, sizeof(cmdBuffer));
    memset(resultBuffer, 0, sizeof(resultBuffer));

    sprintf(cmdBuffer, "ps -p %d -o \"%%cpu,%%mem,rss\" | grep -v MEM", pid);

    printf("%s\n", cmdBuffer);

    cmd_system(cmdBuffer, resultBuffer, sizeof(resultBuffer));

    if (string_is_empty(resultBuffer) > 0) {
        printf("get process '%d' status failed\n", pid);
    }
    else {
        printf("process '%d' status: %s\n", pid,resultBuffer);

        sscanf(resultBuffer,"%f %f %d", &ps->cpuPercent, &ps->memPercent, &ps->memUsage);
        rc = 0;
    }

    return rc;
}
