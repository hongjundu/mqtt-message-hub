/*
 *  File: sysstatus.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/07.
 *  Copyright 2018 All rights reserved.
 */

#pragma once

#include <unistd.h>
#include <time.h>

typedef struct 
{
    int total;
    int used;
    int free;
} sys_mem_status;

typedef struct
{
    float sysLoadAverage1min;  /* system load averages for the past 1 minute */
    float sysLoadAverage5min;  /* system load averages for the past 5 minute */
    float sysLoadAverage15min; /* system load averages for the past 15 minute */
} sys_load_status;

int get_sys_mem_status(sys_mem_status *sms);

time_t get_sys_boot_time();

int get_sys_load(sys_load_status *sls);