/*
 *  File: threadpoolwrapper.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#include "threadpoolwrapper.h"
#include "threadpool.h"
#include "logger.h"

#define THREAD_COUNT     1
#define TASK_QUEUE_COUNT 2048

static threadpool_t *g_threadpool = NULL;

/*
 create threadpool object
*/
int ThreadPoolWrapper_init()
{
    Log_debug("ThreadPoolWrapper_init");

    if (g_threadpool) {
        Log_warn("threadpool instance has been  already created");
        return SUCCESS;
    }

    g_threadpool = threadpool_create(THREAD_COUNT, TASK_QUEUE_COUNT, 0);
    if (!g_threadpool) {
        Log_error("Create threadpool failed");
        return FAILURE;
    }

    Log_info("Create threadpool successfully");

    return SUCCESS;
}

/*
  Returns 1 if threadpool is initialized, otherwise 0
*/
int ThreadPoolWrapper_isInited()
{
    return (g_threadpool != NULL);
}

/*
 add one task to threadpool
*/
int ThreadPoolWrapper_addTask(void (*task)(void*), void *arg, int flags)
{
    Log_debug("ThreadPoolWrapper_addTask");
    int rc = FAILURE;

    if (!ThreadPoolWrapper_isInited()) {
        if (ThreadPoolWrapper_init() < 0) {
            Log_error("Init threadpool failed");
            return FAILURE;
        }
    }

    rc = threadpool_add(g_threadpool, task, arg, 0);

    if (rc == SUCCESS) {
        Log_info("task added successfully");
    }
    else {
        Log_error("task added failed, error: %d", rc);
    }

    return rc;
}

/*
 Destroy threadpool object
*/
int ThreadPoolWrapper_deinit(BOOL graceful)
{
    Log_debug("ThreadPoolWrapper_Deinit graceful: %d", graceful);

    int rc = FAILURE;

    if (!g_threadpool) {
        Log_warn("threadpool was not created");
        return SUCCESS;
    }

    rc = threadpool_destroy(g_threadpool, graceful ? threadpool_graceful : 0);

    Log_info("threadpool deinit completed");

    return rc;
}