/*
 *  File: logger.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#pragma once


#include "zlog.h"

#define LOG_STDOUT

#ifdef LOG_STDOUT
    #define PRINTF(X,...) printf(X,##__VA_ARGS__)
#else
    #define PRINTF(X,...)
#endif

/*
 Init zlog
*/
int  Log_init(char *category);

/*
 Deinit zlog
*/
void Log_deinit();

#define Log_fatal(...)    PRINTF(__VA_ARGS__); PRINTF("\n"); dzlog_fatal(__VA_ARGS__);
#define Log_error(...)    PRINTF(__VA_ARGS__); PRINTF("\n"); dzlog_error(__VA_ARGS__);
#define Log_warn(...)     PRINTF(__VA_ARGS__); PRINTF("\n"); dzlog_warn(__VA_ARGS__);
#define Log_notice(...)   PRINTF(__VA_ARGS__); PRINTF("\n"); dzlog_notice(__VA_ARGS__);
#define Log_info(...)     PRINTF(__VA_ARGS__); PRINTF("\n"); dzlog_info(__VA_ARGS__);
#define Log_debug(...)    PRINTF(__VA_ARGS__); PRINTF("\n"); dzlog_debug(__VA_ARGS__);


#define LOG_CATETORY      "MQTTMSGHUB"