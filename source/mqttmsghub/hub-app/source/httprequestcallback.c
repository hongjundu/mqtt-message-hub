/*
 *  File: httprequestcallback.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#include "httprequestcallback.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <limits.h>
#include "MQTTAsync.h"
#include "common.h"
#include "logger.h"
#include "json.h"
#include "stringutils.h"
#include "devicesn.h"
#include "netutils.h"
#include "mqttclientwrapper.h"
#include "offlinemsgcache.h"
#include "mqttsendreqbodyparser.h"
#include "mqttclientwrapper.h"
#include "processstatus.h"
#include "config.h"
#include "msgcounter.h"

static inline void http_send_response(request *r, char *body)
{
    char    header[64];
    memset(header, 0, sizeof(header));

    httpdSetContentType(r, "application/json\r\n");

    snprintf(header, sizeof(header) - 1,
             "Content-Length: %ld\r\n", strlen(body));
    httpdAddHeader(r, header);
    httpdAddHeader(r, "Access-Control-Allow-Origin:*");

    httpdPrintf(r, "%s", body);
}

static void _http_callback_mqtt_send(httpd *webserver, request *r, BOOL cacheIfSendFailed);
void http_callback_mqtt_send_discard_failed(httpd *webserver, request *r)
{
    Log_debug("http_callback_mqtt_send_discard_failed request body: %s", safe_string(r->request.body));

    _http_callback_mqtt_send(webserver, r, FALSE);
}

void http_callback_mqtt_send_cache_failed(httpd *webserver, request *r)
{
    Log_debug("http_callback_mqtt_send_cache_failed request body: %s", safe_string(r->request.body));

    _http_callback_mqtt_send(webserver, r, TRUE);
}

void http_callback_mqtt_status(httpd *webserver, request *r)
{
    Log_debug("http_callback_mqtt_status request body: %s", safe_string(r->request.body));

    process_status ps;
    memset(&ps, 0, sizeof(process_status));

    get_process_status(getpid(), &ps);

    char cacheRootPath[_POSIX_PATH_MAX] = {0};
    get_config_offline_cache_path(cacheRootPath);

    char responseMessage[1024];
    sprintf(responseMessage,"{\"cpu\":%.2f,\"memory\":%d,\"MQTT_connected_status\":\"%s\",\"offline_cache_capacity\":%d,\"offline_cache_path\":\"%s\",\"offline_message_count\":%d}",
        ps.cpuPercent, ps.memUsage, MQTTClientWrapper_isConnected() ? "connected" : "disconnected", get_offline_cache_capacity(),cacheRootPath, offline_messages_count());

    http_send_response(r,responseMessage);
}

void http_callback_404(httpd *webserver, request *r)
{
    Log_debug("http_callback_404");

    char errorMessage[128];
    sprintf(errorMessage, "{\"%s\":\"%s\",\"%s\":\"request not supported, please check url\"}",JSON_NODE_STATUS_KEY, VALUE_ERROR, JSON_NODE_MESSAGE_KEY);

    http_send_response(r,errorMessage);
}

static void _http_callback_mqtt_send(httpd *webserver, request *r, BOOL cacheIfSendFailed)
{
    Log_debug("_http_callback_mqtt_send request body: %s cacheIfSendFailed: %d", safe_string(r->request.body), cacheIfSendFailed);

    mqtt_send_content content;
    json_object *jobj = NULL;
    char errorMessage[128] = { 0 };
    char responseMessage[512] = { 0 };
    int rc = FAILURE;
    memset(&content, 0, sizeof(content));

    char *serverIP = get_config_listen_server();

    char macAddr[32] = { 0 };
    char clientIp[32] = { 0 };

    // The request ip is from device
    if (string_equals(r->clientAddr, serverIP, TRUE)) {
        if (!string_is_empty(r->request.ipForwardedFor) && !string_equals(r->request.ipForwardedFor, serverIP, TRUE)) {
            strcpy(clientIp, r->request.ipForwardedFor);
        }
    }
    else { // The request ip is not from device
        strcpy(clientIp, r->clientAddr);
    }

    if (!string_is_empty(clientIp)) {
        if (arp_get_mac(clientIp, macAddr) == SUCCESS) {
            Log_info("arp get MAC address %s successfully: %s", clientIp, macAddr);
        }
        else {
            Log_warn("arp get MAC address for %s failed", clientIp);
            *macAddr = 0;
        }    
    }

    rc = parse_mqtt_send_request_body(r->request.body, macAddr, &content, &jobj, errorMessage);
    if (rc == SUCCESS) {
        Log_info("Sending mqtt message topic: %s, payload: %s ...", content.topic, content.payload);
        int sendRetCode = FAILURE;

        if (MQTTClientWrapper_isConnected()) {
            sendRetCode = MQTTClientWrapper_sendMessage(content.topic, content.payload, content.qos);
        } 
        else {
            sendRetCode = MQTTASYNC_DISCONNECTED;
        }


        if (sendRetCode == SUCCESS) {
            Log_info("Send MQTT message successfully");

            sprintf(responseMessage, "{\"%s\":\"%s\"}", JSON_NODE_STATUS_KEY, VALUE_OK);
            http_send_response(r, responseMessage);
        }
        else {
            Log_error("Send MQTT message failed, error %d",sendRetCode);

            char cacheStatusMsg[256] = { 0 };
            char cacheErrMsg[256] =  { 0 };

            if (cacheIfSendFailed) {
                Log_info("Cache offline message");
                strcpy(cacheStatusMsg, "cache status: ");

                struct timespec spec;
                int64_t timestamp_nsec = 0;

                clock_gettime(CLOCK_REALTIME, &spec);
                timestamp_nsec =(int64_t)spec.tv_sec * (int64_t)NANOSECONDS_PER_SECOND + (int64_t)spec.tv_nsec;

                int cacheRC = enqueue_offline_message((char*)json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PLAIN | JSON_C_TO_STRING_NOSLASHESCAPE), content.cache_priority, cacheErrMsg, timestamp_nsec); 
                if (cacheRC == SUCCESS) {
                    strcat(cacheStatusMsg, "message was cached to offline queue");
                }
                else {
                    strcat(cacheStatusMsg, cacheErrMsg);
                }
            }
            else {
                increase_online_discarded_msg_count();
                strcpy(cacheStatusMsg, "discard it");
            }

            sprintf(responseMessage, "{\"%s\":\"%s\",\"%s\":\"Send MQTT message failed, error: %s. %s\"}",
                JSON_NODE_STATUS_KEY, VALUE_ERROR, JSON_NODE_MESSAGE_KEY, MQTTClientWrapper_errorDescription(sendRetCode), cacheStatusMsg);

            http_send_response(r, responseMessage);
        }
    }
    else {
        sprintf(responseMessage, "{\"%s\":\"%s\",\"%s\":\"%s\"}", JSON_NODE_STATUS_KEY, VALUE_ERROR, JSON_NODE_MESSAGE_KEY, errorMessage);
        http_send_response(r, responseMessage);
    }

    if (jobj) {
         json_object_put(jobj); /* free the object */
    }
}