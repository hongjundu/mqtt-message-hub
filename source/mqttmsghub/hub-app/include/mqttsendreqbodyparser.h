/*
 *  File: mqttsendreqbodyparser.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#pragma once

#include "json.h"

typedef struct {
    char *from;
    char *topic;
    char *payload;
    int  qos;
    int  cache_priority;
} mqtt_send_content;

int parse_mqtt_send_request_body(char *body, char *fromMacAddr, mqtt_send_content *mqttSendContent, json_object **outJsonObject, char *errorMessage);