/*
 *  File: filechangewatcher.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#include "filechangewatcher.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include "list.h"
#include "stringutils.h"
#include "filepathutils.h"

typedef struct {
    char path[_POSIX_PATH_MAX];
    time_t modifyTime;
    file_change_notify *notify;
}watch_file;

static void freeListVal(void *val) {
    if (val) {
        watch_file *wf = (watch_file*)val;
        free(wf);
    }
}

file_change_watcher *create_file_change_watcher(int timeInterval)
{
    file_change_watcher *watcher = (file_change_watcher*)malloc(sizeof(file_change_watcher));
    watcher->timeInterval = timeInterval;
    watcher->threadId = 0;
    list_t *list = list_new();
    list->free = freeListVal;
    watcher->list = list;
    return watcher;
}

int delete_file_change_watcher(file_change_watcher *watcher)
{
    if (watcher->list) {
        /*
        list_destroy also deletes its elements
        */
        list_destroy((list_t*)watcher->list);
        watcher->list = NULL;
    }
    free(watcher);

    return 0;
}

int add_watch_file(file_change_watcher *watcher, char *path, file_change_notify *notify)
{
    watch_file *wf = (watch_file*)malloc(sizeof(watch_file));
    strcpy(wf->path, path);
    wf->modifyTime = 0;
    if (file_exists(wf->path)) {
        wf->modifyTime = get_file_modified_time(wf->path);
    }
    wf->notify = notify;

    printf("add_watch_file: %s modifyTime: %lld\n", path, (long long int)wf->modifyTime);

    list_node_t *node = list_node_new(wf);
    list_rpush((list_t*)watcher->list, node);

    return 0;
}

static int file_path_match(void *val1, void *val2) {
    watch_file *wf1 = (watch_file*)val1;
    watch_file *wf2 = (watch_file*)val2;
    return strcmp(wf1->path,wf2->path) == 0;
}

int remove_watch_file(file_change_watcher *watcher, char *path)
{
    list_t *list = (list_t*)watcher->list;

    list->match = file_path_match;

    watch_file wf;
    strcpy(wf.path,path);

    list_node_t *node = list_find(list, &wf);
    if (node) {
        list_remove(list, node);
        return 0;
    }

    return -1;
}

static void *watch_thread_fun(void *data);
int start_watching(file_change_watcher *watcher)
{
    if (watcher->threadId > 0) {
        printf("watch thread already started\n");
        return -1;
    }

    int rc = pthread_create(&watcher->threadId, NULL, watch_thread_fun, watcher);

    if (rc < 0) {
        printf("Failed to create watch thread\n");
        return rc;
    }

    return 0;
}

int stop_watching(file_change_watcher *watcher)
{
    if (watcher->threadId > 0) {
        pthread_kill(watcher->threadId,0);
    }
    else {
        printf("watch not started\n");
    }
    return 0;
}


static void *watch_thread_fun(void *data) {
    file_change_watcher *watcher = (file_change_watcher*)data;

    while (1) {
        list_t *list = (list_t*)watcher->list;
        list_iterator_t *it = list_iterator_new(list, LIST_HEAD);
        list_node_t *node;
        while (node = list_iterator_next(it)) {
            watch_file *wf = (watch_file*)node->val;
            time_t modifyTime = 0;

            if (file_exists(wf->path)) {
                modifyTime = get_file_modified_time(wf->path);
            }

            if (modifyTime == 0) { /* error occurs or file doesn't exist */
                continue;
            }
            else if (modifyTime != wf->modifyTime){
                wf->modifyTime = modifyTime;
                if (wf->notify) {
                    wf->notify(wf->path, wf->modifyTime);
                }
            }
        }

        list_iterator_destroy(it);

        usleep(watcher->timeInterval * 1000 * 1000);
    }
    return 0;
}
