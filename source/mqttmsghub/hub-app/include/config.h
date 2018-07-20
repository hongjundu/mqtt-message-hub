/*
 *  File: config.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#pragma once

char* get_config_mqtt_end_point();

char* get_config_listen_server();

int get_config_listen_port();

int get_config_offline_cache_path(char *path);

int get_config_offline_cache_capacity();
