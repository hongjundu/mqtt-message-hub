/*
 *  File: msgrevhandler.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#include "msgrevhandler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include "stringutils.h"
#include "logger.h"
#include "writepipe.h"
#include "common.h"
#include "base64.h"
#include "devicesn.h"

BOOL msg_received_handler(char *topic, unsigned char *payload, int payloadLen)
{
    Log_debug("msg_received_hanlder topic: %s payloadLen: %d", safe_string(topic), payloadLen);

    char *cmdTopicPrefix = strstr(topic, CAT_TOPIC(MAIN_TOPIC,CMD_SUB_TOPIC));
    if (!cmdTopicPrefix) {
        Log_error("%s is not a valid command topic", topic);
        return FALSE;
    }

    char *splitter = strrchr(topic, '/');
    if (!splitter) {
        Log_error("%s is not a valid command topic", topic);
        return FALSE;
    }

    splitter++;

    if (!splitter) {
        Log_error("%s is not a valid command topic", topic);
        return FALSE;
    }

    char topicTail[64] = { 0 };
    char deviceSNSubJson[64] = { 0 };

    strcpy(topicTail, splitter);
    if (string_equals(topicTail, get_device_sn(), TRUE) ) {
        Log_debug("this command is sent to this device");

        string_upper_case(topicTail);
        sprintf(deviceSNSubJson, "\"%s\":\"%s\",", JSON_NODE_SN_KEY, topicTail);
    }
    else if (string_equals(topicTail, CMD_ALL_SUB_TOPIC, TRUE)) {
        Log_debug("this command is sent to all devices");
    }
    else {
        Log_error("%s is not a valid command topic", topic);
        return FALSE;
    }


    if (payload && payloadLen > 0) {
        /*
         encode as base64 string, as it can be encryped bytes buffer
        */
        unsigned long base64size = calc_base64_encrypted_size(payloadLen);
        char *base64payload = (char*)malloc(base64size);
        base64_encrypt((unsigned char*)payload, payloadLen, base64payload);

        if (string_is_empty(base64payload)) {
            Log_warn("payload is empty");
        }
        else {
            char *pipeBuffer = (char*)malloc(base64size + 1 + strlen(safe_string(topic)) + 256); /* reserve extra memory for JSON tags */

            sprintf(pipeBuffer,"{\"%s\":\"%s\",%s\"%s\":\"%s\"}", JSON_NODE_MQTT_TOPIC_KEY, topic, deviceSNSubJson, JSON_NODE_MQTT_PAYLOAD_KEY, base64payload);
            write_pipe_buffer(pipeBuffer);

            free(pipeBuffer);
        }

        free(base64payload);
    }
    else {
        Log_warn("payload is empty, nothing to to");
    }

    return TRUE;
}