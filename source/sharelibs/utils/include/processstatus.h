/*
 *  File: processstatus.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#pragma once

#include <unistd.h>

typedef struct 
{
    float cpuPercent;
    float memPercent;
    int   memUsage;
} process_status;

pid_t get_process_id(const char *procesName);
int get_process_status(pid_t pid, process_status *ps);