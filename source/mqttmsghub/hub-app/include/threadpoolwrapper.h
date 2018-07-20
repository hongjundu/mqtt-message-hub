/*
 *  File: threadpoolwrapper.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#pragma once

#include "common.h"

/*
 create threadpool object
*/
int ThreadPoolWrapper_init();

/*
  Returns 1 if threadpool is initialized, otherwise 0
*/
int ThreadPoolWrapper_isInited();

/*
 add one task to threadpool
*/
int ThreadPoolWrapper_addTask(void (*task)(void*), void *arg, int flags);

/*
 Destroy threadpool object
*/
int ThreadPoolWrapper_deinit(BOOL graceful);