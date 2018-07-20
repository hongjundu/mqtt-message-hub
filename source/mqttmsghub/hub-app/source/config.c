/*
 *  File: config.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "common.h"
#include "logger.h"
#include "stringutils.h"
#include "filepathutils.h"
#include "ini_parser.h"
#include "ini_parser_helper.h"

#define MQTT_END_POINT_CONFIG_KEY       "mqttserver"
#define SERVER_CONFIG_KEY               "server"
#define PORT_CONFIG_KEY                 "port"
#define CACHE_PATH_CONFIG_KEY           "cachepath"
#define CACHE_CAPACITY_CONFIG_KEY       "cachecapacity"

#define DEFAULT_SERVER                  "127.0.0.1"
#define DEFAULT_PORT                    8066
#define DEFAULT_OFFLINE_CACHE_CAPACITY  1000


static char g_mqttEndPoint[256] = { 0 };
char* get_config_mqtt_end_point()
{
    Log_debug("get_config_mqtt_end_point");

    if (string_is_empty(g_mqttEndPoint)) {
        get_string_from_ini_file(MQTTMSGHUB_INI_FILE_NAME, MQTT_END_POINT_CONFIG_KEY, "", g_mqttEndPoint);
    }

    return g_mqttEndPoint;
}

static char g_listenServer[32] = { 0 };
char *get_config_listen_server()
{
    Log_debug("get_config_listen_server");
    if (string_is_empty(g_listenServer)) {
        get_string_from_ini_file(MQTTMSGHUB_INI_FILE_NAME, SERVER_CONFIG_KEY, DEFAULT_SERVER, g_listenServer);
    }

    return g_listenServer;
}

int get_config_listen_port()
{
    Log_debug("get_config_listen_port");

    int port = get_int_from_ini_file(MQTTMSGHUB_INI_FILE_NAME, PORT_CONFIG_KEY, DEFAULT_PORT);

    if (port <= 0) {
        port = DEFAULT_PORT;
    }

    return port;
}

int get_config_offline_cache_path(char *path)
{
    Log_debug("get_config_offline_cache_path");

    char defaultCacheRootPath[_POSIX_PATH_MAX] = {0};
    sprintf(defaultCacheRootPath, "%s/%s/%s", home_path(),APP_DATA_PATH, MQTT_OFFLINE_MSG_DIR);

    return get_string_from_ini_file(MQTTMSGHUB_INI_FILE_NAME, CACHE_PATH_CONFIG_KEY, defaultCacheRootPath, path);
}

int get_config_offline_cache_capacity()
{
    Log_debug("get_config_offline_cache_capacity");

    return get_int_from_ini_file(MQTTMSGHUB_INI_FILE_NAME, CACHE_CAPACITY_CONFIG_KEY, DEFAULT_OFFLINE_CACHE_CAPACITY);
}