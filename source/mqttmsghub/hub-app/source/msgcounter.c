/*
 *  File: msgcounter.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2018/02.
 *  Copyright 2018 All rights reserved.
 */

#include "msgcounter.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include "logger.h"
#include "ini_parser.h"
#include "stringutils.h"
#include "filepathutils.h"

#define MSG_COUNTER_FILE            "msgcounter.ini"

#define SENT_MSG_COUNT_KEY          "sentcount"
#define ONLINE_DISCARD_COUNT_KEY    "onlinediscardcount"
#define OFFLINE_DISCARD_COUNT_KEY   "offlinediscardcount"
#define OFFLINE_SENT_COUNT_KEY      "offlinesentcount"

static pthread_mutex_t g_sent_msg_count_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_online_discarded_msg_count_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_offline_discarded_msg_count_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_offline_sent_msg_count_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_ini_file_mutex = PTHREAD_MUTEX_INITIALIZER;

static int g_sent_msg_count = 0;
static int g_online_discarded_msg_count = 0;
static int g_offline_discarded_msg_count = 0;
static int g_offline_sent_msg_count = 0;

static void get_msg_count_file_path(char *buffer);
static int _read_total_msg_count(int *sentCount, int *onlineDiscardedCount, int *offlineDiscardedCount,int *offlineSentCount);
static int _save_total_msg_count(int sentCount, int onlineDiscardedCount, int offlineDiscardedCount, int offlineSentCount);

void increase_sent_msg_count()
{
    pthread_mutex_lock(&g_sent_msg_count_mutex);
    g_sent_msg_count++;
    pthread_mutex_unlock(&g_sent_msg_count_mutex);
}

void increase_online_discarded_msg_count()
{
    pthread_mutex_lock(&g_online_discarded_msg_count_mutex);
    g_online_discarded_msg_count++;
    pthread_mutex_unlock(&g_online_discarded_msg_count_mutex);
}

void increase_offline_discarded_msg_count()
{
    pthread_mutex_lock(&g_offline_discarded_msg_count_mutex);
    g_offline_discarded_msg_count++;
    pthread_mutex_unlock(&g_offline_discarded_msg_count_mutex);
}

void increase_offline_sent_msg_count()
{
    pthread_mutex_lock(&g_offline_sent_msg_count_mutex);
    g_offline_sent_msg_count++;
    pthread_mutex_unlock(&g_offline_sent_msg_count_mutex);
}

int get_total_msg_count(int *sentCount, int *onlineDiscardedCount, int *offlineDiscardedCount,int *offlineSentCount)
{
    int rc = FAILURE;

    pthread_mutex_lock(&g_ini_file_mutex);
    rc = _read_total_msg_count(sentCount, onlineDiscardedCount, offlineDiscardedCount,offlineSentCount);
    pthread_mutex_unlock(&g_ini_file_mutex);

    return rc;
}

void save_msg_count_to_file()
{
    if (g_sent_msg_count == 0 
        && g_online_discarded_msg_count == 0 
        && g_offline_discarded_msg_count == 0
        && g_offline_sent_msg_count == 0) {
        return;
    }

    int sentCount = 0, onlineDiscardedCount = 0, offlineDiscardedCount = 0, offlineSentCount = 0;

    pthread_mutex_lock(&g_ini_file_mutex);

    _read_total_msg_count(&sentCount, &onlineDiscardedCount, &offlineDiscardedCount, &offlineSentCount);

    pthread_mutex_lock(&g_sent_msg_count_mutex);
    sentCount += g_sent_msg_count;
    g_sent_msg_count = 0;
    pthread_mutex_unlock(&g_sent_msg_count_mutex);

    pthread_mutex_lock(&g_online_discarded_msg_count_mutex);
    onlineDiscardedCount += g_online_discarded_msg_count;
    g_online_discarded_msg_count = 0;
    pthread_mutex_unlock(&g_online_discarded_msg_count_mutex);

    pthread_mutex_lock(&g_offline_discarded_msg_count_mutex);
    offlineDiscardedCount += g_offline_discarded_msg_count;
    g_offline_discarded_msg_count = 0;
    pthread_mutex_unlock(&g_offline_discarded_msg_count_mutex);

    pthread_mutex_lock(&g_offline_sent_msg_count_mutex);
    offlineSentCount += g_offline_sent_msg_count;
    g_offline_sent_msg_count = 0;
    pthread_mutex_unlock(&g_offline_sent_msg_count_mutex);

    _save_total_msg_count(sentCount, onlineDiscardedCount, offlineDiscardedCount, offlineSentCount);

    pthread_mutex_unlock(&g_ini_file_mutex);
}

void clear_msg_count_file()
{
    pthread_mutex_lock(&g_ini_file_mutex);
    _save_total_msg_count(0, 0, 0, 0);
    pthread_mutex_unlock(&g_ini_file_mutex);
}

/*************************************************************************************
 Private Helpers
**************************************************************************************/
static int _read_total_msg_count(int *sentCount, int *onlineDiscardedCount, int *offlineDiscardedCount, int *offlineSentCount)
{
    *sentCount = 0;
    *onlineDiscardedCount = 0;
    *offlineDiscardedCount = 0;
    *offlineSentCount = 0;

    char filePath[_POSIX_PATH_MAX] = { 0 };
    get_msg_count_file_path(filePath);

    struct ini_parser *parser = new_ini_parser();
    if (!parser) {
        Log_error("%s", "create ini parser failed\n");
        return FAILURE;
    }

    int size = parser->parse_file(parser,filePath);
    if (size <= 0) {
        Log_warn("Nothing was read from ini file: %s\n", filePath);
        parser->deletor(parser);
        return FAILURE;
    }

    char *str = parser->value(parser, SENT_MSG_COUNT_KEY);
    if (!string_is_empty(str)) {
        if (string_is_number(str)) {
            *sentCount = atoi(str);
        }
    }

    str = parser->value(parser, ONLINE_DISCARD_COUNT_KEY);
    if (!string_is_empty(str)) {
        if (string_is_number(str)) {
            *onlineDiscardedCount = atoi(str);
        }
    }

    str = parser->value(parser, OFFLINE_DISCARD_COUNT_KEY);
    if (!string_is_empty(str)) {
        if (string_is_number(str)) {
            *offlineDiscardedCount = atoi(str);
        }
    }

    str = parser->value(parser, OFFLINE_SENT_COUNT_KEY);
    if (!string_is_empty(str)) {
        if (string_is_number(str)) {
            *offlineSentCount = atoi(str);
        }
    }

    parser->deletor(parser);

    return SUCCESS;
}

static int _save_total_msg_count(int sentCount, int onlineDiscardedCount, int offlineDiscardedCount, int offlineSentCount)
{
    char filePath[_POSIX_PATH_MAX] = { 0 };
    get_msg_count_file_path(filePath);

    struct ini_parser *parser = new_ini_parser();
    if (!parser) {
        Log_error("%s", "create ini parser failed\n");
        return FAILURE;
    }

    parser->parse_file(parser,filePath);

    char str[64] = { 0 };
    sprintf(str, "%d", sentCount);
    parser->set_value(parser, SENT_MSG_COUNT_KEY, str);

    sprintf(str, "%d", onlineDiscardedCount);
    parser->set_value(parser, ONLINE_DISCARD_COUNT_KEY, str);

    sprintf(str, "%d", offlineDiscardedCount);
    parser->set_value(parser, OFFLINE_DISCARD_COUNT_KEY, str);

    sprintf(str, "%d", offlineSentCount);
    parser->set_value(parser, OFFLINE_SENT_COUNT_KEY, str);

    parser->save_to_file(parser, filePath);

    parser->deletor(parser);

    return SUCCESS;
}

static void get_msg_count_file_path(char *buffer)
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
    strcat(buffer, MSG_COUNTER_FILE);
}
