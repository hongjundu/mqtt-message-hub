/*
 *  File: mqttsendreqbodyparser.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#include "mqttsendreqbodyparser.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include "common.h"
#include "json_helper.h"
#include "logger.h"
#include "stringutils.h"
#include "devicesn.h"

int parse_mqtt_send_request_body(char *body, char *fromMacAddr, mqtt_send_content *mqttSendContent, json_object **outJsonObject, char *errorMessage)
{
    Log_debug("parse_mqtt_send_request_body %s", body);

    time_t timestamp = 0;
    json_object *jobj = json_tokener_parse(body);
    *outJsonObject = jobj;

    if (!jobj) {
        sprintf(errorMessage, "Parse JSON error: %s", json_util_get_last_err());
        Log_error("%s", errorMessage);
        return FAILURE;
    }

    memset(mqttSendContent, 0, sizeof(mqtt_send_content));

    if (json_helper_get_string(jobj, JSON_NODE_FROM_MODULE_KEY, &mqttSendContent->from, errorMessage) != JSON_HELPER_OK) {
        /* ignore the error as it is not important */
        Log_warn("%s, ignore it", errorMessage);
        *errorMessage = 0;
    }

    if (json_helper_get_int(jobj, JSON_NODE_MQTT_QOS_KEY, &mqttSendContent->qos, errorMessage) != JSON_HELPER_OK) {
        Log_debug("%s, Use default value QOS 0", errorMessage);
        mqttSendContent->qos = QOS0;
        *errorMessage = 0;
    }

    if (json_helper_get_int(jobj, JSON_NODE_MQTT_CACHE_PRIORITY, &mqttSendContent->cache_priority, errorMessage) != JSON_HELPER_OK) {
        Log_debug("%s, Use default cache_priority %d, only vaild for offlinecache API", errorMessage, VALUE_CACHE_PRIORITY_DEFAULT);
        mqttSendContent->cache_priority = VALUE_CACHE_PRIORITY_DEFAULT;
        *errorMessage = 0;
    }

    if (json_helper_get_string(jobj, JSON_NODE_MQTT_TOPIC_KEY, &mqttSendContent->topic, errorMessage) != JSON_HELPER_OK) {
        Log_error(errorMessage);
        return FAILURE;
    }

    if (string_is_empty(mqttSendContent->topic)) {
        strcpy(errorMessage,"topic is empty");
        Log_error("%s", errorMessage);
        return FAILURE;
    }

    json_object *jsubobj = NULL;
    if (json_object_object_get_ex(jobj, JSON_NODE_MQTT_PAYLOAD_KEY, &jsubobj)) {
        if (json_object_is_type(jsubobj, json_type_string)) {
            mqttSendContent->payload = (char*)json_object_get_string(jsubobj);
            if (string_is_empty(mqttSendContent->payload)) {
                strcpy(errorMessage, "payload is empty");
                Log_error("%s", errorMessage);
                return FAILURE;
            }
        }
        else if (json_object_is_type(jsubobj, json_type_object)) {
            Log_info("payload is in JSON format");
            /*
             add device sn, mac, time to payload object if they are not present
            */
            if (!json_object_object_get_ex(jsubobj, JSON_NODE_SN_KEY, NULL)) {
                Log_info("add 'sn' field");
                json_object_object_add(jsubobj, JSON_NODE_SN_KEY, json_object_new_string(get_device_sn()));
            }

            if (!string_is_empty(fromMacAddr) && !json_object_object_get_ex(jsubobj, JSON_NODE_MAC_KEY, NULL)) {
                Log_info("add 'mac' field");
                json_object_object_add(jsubobj, JSON_NODE_MAC_KEY, json_object_new_string(fromMacAddr));
            }

            if (!json_object_object_get_ex(jsubobj, JSON_NODE_TIME_KEY, NULL)) {
                Log_info("add 'time' field");
                timestamp = time(NULL);
                json_object_object_add(jsubobj, JSON_NODE_TIME_KEY, json_object_new_int64(timestamp));
            }
            mqttSendContent->payload = (char*)json_object_to_json_string_ext(jsubobj, JSON_C_TO_STRING_PLAIN | JSON_C_TO_STRING_NOSLASHESCAPE);
        }
        else {
            sprintf(errorMessage, "node '%s' is not a string or JSON object", JSON_NODE_MQTT_PAYLOAD_KEY);
            Log_error("%s", errorMessage);
            return FAILURE;
        }
    }
    else {
        sprintf(errorMessage,"no json node '%s'", JSON_NODE_MQTT_PAYLOAD_KEY);
        Log_error("%s", errorMessage);
        return FAILURE;
    }

    return SUCCESS;
}