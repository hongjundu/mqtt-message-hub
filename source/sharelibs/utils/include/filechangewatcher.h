/*
 *  File: filechangewatcher.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#pragma once

#include <pthread.h>

typedef void *file_change_notify(char *path, time_t modifyTime);

typedef struct {
    void *list;
    int timeInterval; /* in seconds */
    pthread_t threadId;
}file_change_watcher;

file_change_watcher *create_file_change_watcher(int timeInterval);

int delete_file_change_watcher(file_change_watcher *watcher);

int add_watch_file(file_change_watcher *watcher, char *path, file_change_notify *notify);

int remove_watch_file(file_change_watcher *watcher, char *path);

int start_watching(file_change_watcher *watcher);

int stop_watching(file_change_watcher *watcher);


