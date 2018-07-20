/*
 *  File: writepipe.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#include "writepipe.h"
#include <unistd.h>  
#include <stdlib.h>  
#include <pthread.h>
#include <fcntl.h>  
#include <limits.h> 
#include <sys/types.h>  
#include <sys/stat.h>  
#include <stdio.h>  
#include <errno.h>
#include <string.h>  
#include "common.h"
#include "logger.h"
#include "threadpoolwrapper.h"
#include "filepathutils.h"
#include "stringutils.h"

#define OPEN_MODE        (O_WRONLY | O_NONBLOCK)
#define BUFFER_SIZE      PIPE_BUF

static pthread_mutex_t g_write_pipe_mutex = PTHREAD_MUTEX_INITIALIZER;

static int g_pipe_fd = 0;

static void get_fifo_file_path(char *buffer);
static void write_pipe_thread_fun(void *arg);

int open_write_pipe()
{
    Log_info("init_write_pipe");

    char fifoFilePath[_POSIX_PATH_MAX] = { 0 };
    int rc = FAILURE; 
  
    get_fifo_file_path(fifoFilePath);
    Log_info("FIFO file path: %s", fifoFilePath);

    if(!file_exists(fifoFilePath))  
    {   
        rc = mkfifo(fifoFilePath, 0777);  
        if(rc < 0)  
        {  
            Log_error("Could not create FIFO %s, error: %d %s", fifoFilePath, ERRNO_ERRORSTR);  
            return rc;
        }  
    }  
  
    Log_info("Process %d opening FIFO (O_WRONLY | O_NONBLOCK) %s", getpid(), fifoFilePath);  
    g_pipe_fd = open(fifoFilePath, OPEN_MODE);  
    if (g_pipe_fd < 0) {
        Log_error("Open FIFO failed, error: %d %s", ERRNO_ERRORSTR);
        return g_pipe_fd;
    }

    return SUCCESS;
}

void write_pipe_buffer(char *buffer)
{
    Log_info("write_pipe_buffer %s len: %d", buffer, strlen(buffer));

    if (string_is_empty(buffer)) {
        return;
    }

    char *arg = (char*)malloc(strlen(buffer) + 1);
    strcpy(arg, buffer);

    ThreadPoolWrapper_addTask(write_pipe_thread_fun, arg, 0);
}

int close_write_pipe()
{
    Log_info("close_write_pipe");

    if (g_pipe_fd > 0) {
        close(g_pipe_fd);
    }
    else {
        Log_info("FIFO is not open")
    }
    return SUCCESS;
}


/*************************************************************************************
 Private Helpers
**************************************************************************************/

static void get_fifo_file_path(char *buffer)
{
    char appDataPath[_POSIX_PATH_MAX] = { 0 };
    strcpy(appDataPath, home_path());
    strcat(appDataPath, "/");
    strcat(appDataPath, APP_DATA_PATH);
    
    if (!path_exists(appDataPath)) {
        if (mkdirp(appDataPath, 0700) < 0) {
            Log_error("mkdir '%s' failed", appDataPath);
        }
    }

    strcpy(buffer, appDataPath);
    strcat(buffer, "/");
    strcat(buffer, FIFO_FILE_NAME);
}


static void write_pipe_thread_fun(void *arg)
{
    Log_debug("write_pipe_thread_fun");

    char *buffer = (char*)arg;

    Log_debug("arg: %s", buffer);

    pthread_mutex_lock(&g_write_pipe_mutex);


    if (g_pipe_fd <= 0) {
        Log_notice("FIFO is not open, open FIFO...");
        if (open_write_pipe() < 0) {
            Log_error("Open write FIFO failed");
            goto EXIT;
        }
    }

    Log_info("Writing buffer [%s] to FIFO...", buffer);

    int size = write(g_pipe_fd, buffer, strlen(buffer));
    // sleep 0.05 second, to void FIFO data lost
    usleep(50 * 1000);

    Log_info("Write FIFO finished, size: %d", size);

    EXIT:
    pthread_mutex_unlock(&g_write_pipe_mutex);
    free(buffer);
}
