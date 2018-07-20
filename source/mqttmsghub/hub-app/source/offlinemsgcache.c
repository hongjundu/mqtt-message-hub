/*
 *  File: offlinemsgcache.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#include "offlinemsgcache.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <memory.h>
#include <limits.h>  
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include "list.h"
#include "common.h"
#include "config.h"
#include "logger.h"
#include "filepathutils.h"
#include "stringutils.h"
#include "mqttclientwrapper.h"
#include "mqttsendreqbodyparser.h"
#include "msgcounter.h"

#define CACHE_FILE_SUFFIX    ".cache"
#define CACHE_INFO_SEPARATOR '-'

typedef struct {
    int cache_priority;
    int64_t timestamp;
}cache_file_header;

#define LIST_NODE_FREE(node) \
    if (node) { \
        freeListVal(node->val);\
        LIST_FREE(node); \
    }
#define LIST_NODE_VALUE(node) ((node && node->val) ? (cache_file_header*)node->val : 0)

static list_t *g_list = NULL;
static int g_cacheCapacity = 1000;

static pthread_mutex_t g_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_async_sending_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t g_asyncSendThreadId = 0;
static BOOL g_isAsyncSending = FALSE;

/*************************************************************************************
 Publics
**************************************************************************************/
static int init_list();
int offline_messages_count();
static void freeListVal(void *val);
static int remove_one_lowest_priority_message();
static int cache_offline_message(char *message, int cache_priority, char *errorMsg, int64_t timestamp);
static char *cache_root_path();
static BOOL is_queue_full();
static void get_cache_file_header_from_file_base(char *fileBaseName, int *cache_priority, int64_t *timestamp);

int enqueue_offline_message(char *message, int cache_priority, char *errorMsg, int64_t timestamp)
{
    Log_debug("enqueue_offline_message message: %s cache_priority: %d timestamp: %lld", safe_string(message), cache_priority, timestamp);

    if (!g_list) {
        init_list();
    }

    return cache_offline_message(message, cache_priority, errorMsg, timestamp);
}

void set_offline_cache_capacity(int capacity)
{
    Log_debug("set_offline_cache_capacity %d", capacity);
    g_cacheCapacity = capacity;
}

/*
 returns cache capacity
*/
int get_offline_cache_capacity()
{
    return g_cacheCapacity;
}

int offline_messages_count()
{
    int count = 0;

    pthread_mutex_lock(&g_list_mutex);
    count = g_list ? g_list->len : 0;
    pthread_mutex_unlock(&g_list_mutex);

    return count;
}

static void *send_messages_thread_fun(void* data);
int asyn_send_offline_messages()
{
    Log_debug("asyn_send_offline_messages");

    int rc = FAILURE;

    if (offline_messages_count() == 0) {
        Log_info("No offline messages");
        return SUCCESS;
    }

    BOOL isSending = FALSE;

    pthread_mutex_lock(&g_async_sending_mutex);
    isSending = g_isAsyncSending;
    pthread_mutex_unlock(&g_async_sending_mutex);

    if (isSending) {
        Log_info("Sending thread is running... return");
        return 0;
    }

    rc = pthread_create(&g_asyncSendThreadId, NULL, send_messages_thread_fun, 0);

    if (rc < 0) {
        Log_error("Failed to create message send thread, error %d %s",ERRNO_ERRORSTR);
        return rc;
    }

    return SUCCESS;
}

/*
 load cache from disk
*/

static int filter_offline_cache_file(const struct dirent *d) {
    char filePath[_POSIX_PATH_MAX];
    sprintf(filePath, "%s/%s",cache_root_path(), d->d_name);
    if (!is_file(filePath)) {
        return 0;
    }
    char suffix[_POSIX_PATH_MAX];
    get_file_suffix(d->d_name,suffix);
    return string_equals(suffix, CACHE_FILE_SUFFIX, TRUE);
}

static int offline_cache_file_compare(const struct dirent **d1, const struct dirent **d2)
{
    char fileBaseName1[_POSIX_PATH_MAX];
    char fileBaseName2[_POSIX_PATH_MAX];

    get_file_base_name((*d1)->d_name, fileBaseName1);
    get_file_base_name((*d2)->d_name, fileBaseName2);

    int cache_priority1 = VALUE_CACHE_PRIORITY_DEFAULT;
    int cache_priority2 = VALUE_CACHE_PRIORITY_DEFAULT;

    int64_t timestamp1 = 0;
    int64_t timestamp2 = 0;

    get_cache_file_header_from_file_base(fileBaseName1, &cache_priority1, &timestamp1);
    get_cache_file_header_from_file_base(fileBaseName2, &cache_priority2, &timestamp2);

    if (timestamp1 > timestamp2) {
        return 1;
    }
    else if (timestamp1 < timestamp2) {
        return -1;
    }
    return 0;
}

int load_offline_messages()
{
    Log_debug("load_offline_messages from path: %s", cache_root_path());

    struct dirent **namelist = NULL;
    int fileCount = scandir(cache_root_path(), &namelist, filter_offline_cache_file, offline_cache_file_compare);
    if (fileCount < 0) {
        Log_info("Nothing loaded");
        return 0;
    }

    if (!g_list) {
        init_list();
    }

    int addedCount = 0;
    int i = 0;

    for(; i < fileCount; i++) {
        Log_info("Cache File %d: %s", i + 1, namelist[i]->d_name);

        char fileBaseName[_POSIX_PATH_MAX];
        get_file_base_name(namelist[i]->d_name, fileBaseName);

        if (!string_is_empty(fileBaseName)) {
            int cache_priority = -1;
            int64_t timestamp = 0;

            get_cache_file_header_from_file_base(fileBaseName, &cache_priority, &timestamp);

            if (timestamp > 0) {
                cache_file_header *val = (cache_file_header*)malloc(sizeof(cache_file_header));
                val->cache_priority = cache_priority;
                val->timestamp = timestamp;

                list_node_t *node = list_node_new(val);

                Log_info("adding %s to queue", namelist[i]->d_name);
                pthread_mutex_lock(&g_list_mutex);
                list_rpush(g_list, node);
                pthread_mutex_unlock(&g_list_mutex);

                addedCount++;
            }
            else {
                // invalid cache file
                char fullPath[_POSIX_PATH_MAX] = { 0 };
                sprintf(fullPath,"%s/%s", cache_root_path(),namelist[i]->d_name);
                unlink(fullPath);
            }
        }

        free(namelist[i]);
    }

    Log_info("%d offline caches", addedCount);

    free(namelist);

    return addedCount;
}

/*************************************************************************************
 Privates
**************************************************************************************/

static void freeListVal(void *val)
{
    if (val) {
        cache_file_header *cfh = (cache_file_header*)val;
        free(cfh);
    }
}

static int init_list()
{
    Log_info("init_list");

    if (g_list) {
        Log_info("list has been already initialized");
        return SUCCESS;
    }

    pthread_mutex_lock(&g_list_mutex);
    g_list = list_new();
    g_list->free = freeListVal;
    pthread_mutex_unlock(&g_list_mutex);

    return SUCCESS;
}

static BOOL is_queue_full() {
    BOOL full = FALSE;
    pthread_mutex_lock(&g_list_mutex);
    int count = g_list ? g_list->len : 0;
    full = (count >= g_cacheCapacity);
    pthread_mutex_unlock(&g_list_mutex);
    return full;
}

static char g_cacheRootPath[_POSIX_PATH_MAX] = { 0 };
static char *cache_root_path() {
    if (string_is_empty(g_cacheRootPath)) {
        if (get_config_offline_cache_path(g_cacheRootPath) != SUCCESS) {
            sprintf(g_cacheRootPath, "%s/%s/%s", home_path(),APP_DATA_PATH, MQTT_OFFLINE_MSG_DIR);
        }
        if (!path_exists(g_cacheRootPath)) {
            if (mkdirp(g_cacheRootPath, 0700) < 0) {
                memset(g_cacheRootPath, 0, sizeof(g_cacheRootPath));
                Log_error("mkdir %s failed", g_cacheRootPath);
            }
        }
    }
    return g_cacheRootPath;
}

static void get_cache_file_path(char *buffer, int cache_priority, int64_t timestamp)
{
    if (cache_priority >= 0) {
        sprintf(buffer,"%s/%lld-%d%s", cache_root_path(), timestamp, cache_priority, CACHE_FILE_SUFFIX);
    }
    else {
        sprintf(buffer,"%s/%lld%s", cache_root_path(), timestamp,CACHE_FILE_SUFFIX);
    }
}

static void get_cache_file_header_from_file_base(char *fileBaseName, int *cache_priority, int64_t *timestamp)
{
    char fileBaseNameCopy[_POSIX_PATH_MAX] = { 0 };
    strcpy(fileBaseNameCopy, fileBaseName);

    char *separator = strchr(fileBaseNameCopy, CACHE_INFO_SEPARATOR);
    if (separator) {
        char *p = separator;
        p++;
        if (*p) {
            *cache_priority = atoi(p);
        }
        else {
            *cache_priority = VALUE_CACHE_PRIORITY_DEFAULT;
        }

        *separator = 0;
        *timestamp = atoll(fileBaseNameCopy);
    }
    else {
        *cache_priority = VALUE_CACHE_PRIORITY_DEFAULT;
        *timestamp = atoll(fileBaseNameCopy);
    }
}

static BOOL can_message_enter_offine_queue(int cache_priority) {
    Log_debug("can_message_enter_offine_queue: %d", cache_priority);

    BOOL ret = FALSE;
    pthread_mutex_lock(&g_list_mutex);
    int count = g_list ? g_list->len : 0;

    /* offline queue is not full */
    if (count < g_cacheCapacity) {
        ret = TRUE;
        goto EXIT;
    }

    list_iterator_t *it = list_iterator_new(g_list, LIST_HEAD);
    list_node_t *node = list_iterator_next(it);
    while (node) {
        cache_file_header *cfh = LIST_NODE_VALUE(node);

        /* offline queue is full but has lower (same) prioirty message */
        if (cfh && cfh->cache_priority <= cache_priority) {
            ret = TRUE;
            break;
        }
        node = list_iterator_next(it);
    }
    list_iterator_destroy(it);

    EXIT:
    pthread_mutex_unlock(&g_list_mutex);
    return ret;
}

static int remove_one_lowest_priority_message() 
{
    Log_debug("remove_one_lowest_priority_message total: %d",g_list ? g_list->len : 0);

    int lowest_priority = INT_MAX;
    list_iterator_t *it = NULL;
    list_node_t *node = NULL;
    list_node_t *foundNode = NULL;
    cache_file_header *cfh = NULL;

    /*
     1st round, go throuth and calculate the lowest priority
    */

    it = list_iterator_new(g_list, LIST_HEAD);
    node = list_iterator_next(it);
    while (node) {
        cfh = LIST_NODE_VALUE(node);

        if (cfh && cfh->cache_priority < lowest_priority) {
            lowest_priority = cfh->cache_priority;
        }
        node = list_iterator_next(it);
    }
    list_iterator_destroy(it);
    it = NULL;
    node = NULL;

    /*
     2nd round, go throuth and remove 
    */

    it = list_iterator_new(g_list, LIST_HEAD);
    node = list_iterator_next(it);
    while (node) {
        cfh = LIST_NODE_VALUE(node);
        if (cfh && cfh->cache_priority == lowest_priority) {
            foundNode = node;
            break;
        }
        node = list_iterator_next(it);
    }
    list_iterator_destroy(it);
    it = NULL;
    node = NULL;

    if (foundNode) {
        cfh = LIST_NODE_VALUE(foundNode);

        char filePath[_POSIX_PATH_MAX] = { 0 };
        get_cache_file_path(filePath, cfh->cache_priority, cfh->timestamp);
        if (file_exists(filePath)) {
            unlink(filePath);
            Log_info("cache file has been %s removed successfully", filePath);
        }
        else {
            Log_warn("cache file: '%s' doesn't exist.", filePath);
        }

        /*
         don't worry, memory of node and node->val will be freed by list_remove
        */
        list_remove(g_list, foundNode);

        increase_offline_discarded_msg_count();

        Log_info("cache with priority %d has been removed from memory successfully", lowest_priority);

    }
    else {
        Log_warn("No node was removed, not found lowest priority node");
    }

    return SUCCESS;
}

static int cache_offline_message(char *message, int cache_priority, char *errorMsg, int64_t timestamp) 
{
    Log_debug("cache_offline_message message: %s, cache_priority: %d timestamp: %lld", safe_string(message), cache_priority, timestamp);

    if (!can_message_enter_offine_queue(cache_priority)) {
        strcpy(errorMsg, "offline queue is full, and no message with lower or same priority exists, discard it");
        Log_info("%s", errorMsg);

        increase_offline_discarded_msg_count();

        return FAILURE;
    }

    if (timestamp <= 0) {
        sprintf(errorMsg, "timestamp %lld is invalid, discard it", timestamp);
        Log_warn("%s", errorMsg);

        return FAILURE;
    }

    if (string_is_empty(message)) {
        strcpy(errorMsg, "message is empty, discard it");
        Log_warn("%s", errorMsg);

        return FAILURE;
    }

    char filePath[_POSIX_PATH_MAX] = { 0 };
    size_t writeSize = 0;

    get_cache_file_path(filePath, cache_priority, timestamp);

    if (string_is_empty(filePath)) {
        strcpy(errorMsg, "file path is empty, discard it");
        Log_error("%s", errorMsg);
        return FAILURE;
    }

    Log_info("Save message to file '%s' ...", filePath);

    writeSize = write_file_string(message, sizeof(char), strlen(message), filePath, "w");
    if (writeSize < 0) {
        sprintf(errorMsg, "write cache file failed, error %d %s", ERRNO_ERRORSTR);
        Log_error("%s", errorMsg);

        if (file_exists(filePath)) {
            unlink(filePath);
        }
        return FAILURE;
    }

    else if (writeSize == 0) {
        strcpy(errorMsg,"nothing was written to cache file");
        Log_error("%s", errorMsg);

        if (file_exists(filePath)) {
            unlink(filePath);
        }
        return FAILURE;
    }

    cache_file_header *val = (cache_file_header*)malloc(sizeof(cache_file_header));
    val->cache_priority = cache_priority;
    val->timestamp = timestamp;

    list_node_t *node = list_node_new(val);

    Log_info("adding to memory cache...");

    pthread_mutex_lock(&g_list_mutex);
    int len = g_list ? g_list->len : 0;
    while(len >= g_cacheCapacity) {
        remove_one_lowest_priority_message();
        len = g_list ? g_list->len : 0;
    }
    list_rpush(g_list, node);
    pthread_mutex_unlock(&g_list_mutex);

    Log_notice("Offline message count: %d", offline_messages_count());

    return SUCCESS;
}

static void *send_messages_thread_fun(void* data) {
    Log_debug("send_messages_thread_fun");

    pthread_mutex_lock(&g_async_sending_mutex);
    if (g_isAsyncSending) {
        pthread_mutex_unlock(&g_async_sending_mutex);
        return 0;
    }
    g_isAsyncSending = TRUE;
    pthread_mutex_unlock(&g_async_sending_mutex);

    int count = offline_messages_count();
    list_node_t *node = NULL;
    json_object *jobj = NULL;
    mqtt_send_content content;
    char errorMessage[128];
    int rc = -1;

    while(count > 0) {
        if (!MQTTClientWrapper_isConnected()) {
            Log_warn("MQTT connection is lost while async sending offline messages, STOP");
            break;
        }

        pthread_mutex_lock(&g_list_mutex);
        node = list_lpop(g_list);
        pthread_mutex_unlock(&g_list_mutex);

        if (node) {
            cache_file_header *cfh = LIST_NODE_VALUE(node);
            char filePath[_POSIX_PATH_MAX] = { 0 };
            get_cache_file_path(filePath, cfh->cache_priority, cfh->timestamp);

            if (file_exists(filePath)) {
                char *buffer = NULL;
                size_t readSize = read_file_string(&buffer,filePath);
                if (readSize > 0) {
                    Log_debug("Read cache file: %s", buffer);

                    memset(&content, 0, sizeof(content));
                    rc = parse_mqtt_send_request_body(buffer, NULL, &content, &jobj, errorMessage);
                    if (rc == SUCCESS) {
                        int sendRetCode = MQTTClientWrapper_sendMessage(content.topic, content.payload, content.qos);

                        if (sendRetCode == SUCCESS) {
                            Log_info("Send cache MQTT message successfully");
                            increase_offline_sent_msg_count();
                            unlink(filePath);
                        }
                        else {
                            Log_error("Send Cache MQTT message failed, error: %s", MQTTClientWrapper_errorDescription(sendRetCode));

                            /*
                             Put back to offline messaeg queue if not full, otherwise discard.
                            */
                            BOOL isFull = FALSE;
                            pthread_mutex_lock(&g_list_mutex);
                            int len = (g_list ? g_list->len : 0);
                            if (len < g_cacheCapacity) {
                                list_lpush(g_list, node);
                                node = NULL; /* avoid to be freed */
                            }
                            else {
                                isFull = TRUE;
                            }
                            pthread_mutex_unlock(&g_list_mutex);

                            if (isFull) {
                                Log_warn("Offline message queue is full, discard send failed message");
                            }
                            else {
                                Log_warn("Put back to offline messsage queue");
                            }
                        }
                    }
                    else {
                        Log_error("parse_mqtt_send_request_body failed, %d", rc);
                        unlink(filePath);
                    }

                    if (jobj) {
                        json_object_put(jobj); /* free the json object */
                    }

                    free(buffer); /* free buffer allocated by read_file_string */
                }
                else {
                    Log_error("Read file failed, read size: %d", readSize);
                    unlink(filePath);
                }
            }
            else {
                Log_warn("Cache file '%s' doesn't exist", filePath);
            }

            if (node) {
                LIST_NODE_FREE(node);
            }
        }

        count = offline_messages_count();
    }


    pthread_mutex_lock(&g_async_sending_mutex);
    g_isAsyncSending = FALSE;
    pthread_mutex_unlock(&g_async_sending_mutex);

    return 0;
}