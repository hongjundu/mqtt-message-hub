/*
 *  File: mqttclientwrapper.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#include "mqttclientwrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <time.h>
#include "logger.h"
#include "devicesn.h"
#include "secdef.h"
#include "config.h"
#include "MQTTAsync.h"
#include "common.h"
#include "stringutils.h"
#include "filepathutils.h"
#include "msgcounter.h"

#ifdef PAYLOAD_ENCRYPT
#include "cipher.h"
#endif

#define CLIENTIDPREFIX        "MQTTCLIENT-"
#define KEEPALIVEINTERVAL     10    /* MQTT keep alive, in seconds */
#define DEFAULT_SEND_MSG_QOS  QOS0
#define DEFAULT_SUBSCRIBE_QOS MAXQOS
#define CONNECT_TIMEOUT       10    /* MQTT client connection timeout, in secondes */
#define CLEANSESSION          0     /* Keep session, support offline messages */ 
#define RETRY_INTERVAL        10    /* MQTT retry connect interval, in secondes */
#define MIN_RETRY_INTERVAL    10    /* MQTT min retry connect interval, in secondes */
#define MAX_RETRY_INTERVAL    60    /* MQTT max retry connect interval, in secondes */

static pthread_mutex_t g_connected_status_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_subscribed_status_mutex = PTHREAD_MUTEX_INITIALIZER;

struct MQTTClientStatus
{
    MQTTAsync client;
    BOOL connected;
    BOOL hasEverConnected;
    BOOL subscribed;
    MQTTClientWrapper_messageReceivedCallback *messageReceivedCallback;
    MQTTClientWrapper_connectedCallback *connectedCallback;

} mqttClientStatus =  { NULL, FALSE, FALSE, FALSE, NULL, NULL };


/*************************************************************************************
 MQTT client callbacks
**************************************************************************************/

int MQTTClientWrapper_subscribe();
static void on_connect(void* context, MQTTAsync_successData* response)
{
    Log_debug("on_connect");
    Log_info("MQTT connect successful");

    pthread_mutex_lock(&g_connected_status_mutex);
    if (mqttClientStatus.connected) {
        Log_info("last connected status is TRUE, return");
        pthread_mutex_unlock(&g_connected_status_mutex);
        return;
    }
    
    mqttClientStatus.connected = TRUE;
    pthread_mutex_unlock(&g_connected_status_mutex);

    mqttClientStatus.hasEverConnected = TRUE;

    if (mqttClientStatus.connectedCallback) {
        mqttClientStatus.connectedCallback();
    }

    MQTTClientWrapper_subscribe();
}

static void on_connect_failure(void* context, MQTTAsync_failureData* response)
{
    Log_debug("on_connect_failure");

    Log_error("MQTT connect failed, error: %d message: %s", response ? response->code : -1, (response && response->message) ? response->message : "");

    pthread_mutex_lock(&g_connected_status_mutex);
    mqttClientStatus.connected = FALSE;
    pthread_mutex_unlock(&g_connected_status_mutex);
}

static void on_disconnect(void* context, MQTTAsync_successData* response)
{
    Log_debug("on_disconnect");

    pthread_mutex_lock(&g_connected_status_mutex);
    mqttClientStatus.connected = FALSE;
    pthread_mutex_unlock(&g_connected_status_mutex);

    pthread_mutex_lock(&g_subscribed_status_mutex);
    mqttClientStatus.subscribed = FALSE;
    pthread_mutex_unlock(&g_subscribed_status_mutex);
}

static void on_disconnect_failure(void* context, MQTTAsync_failureData* response)
{
    Log_debug("on_disconnect_failure");
    Log_error("MQTT disconnect failed, error: %d", response ? response->code : -1);
}

static void fill_connect_opts(MQTTAsync_connectOptions *conn_opts);

int MQTTClientWrapper_connect();
static void on_connection_lost(void *context, char *cause)
{
    Log_debug("on_connection_lost cause: %s", safe_string(cause));
    Log_warn("MQTT connection lost, cause: %s", safe_string(cause));

    pthread_mutex_lock(&g_connected_status_mutex);
    mqttClientStatus.connected = FALSE;
    pthread_mutex_unlock(&g_connected_status_mutex);

    pthread_mutex_lock(&g_subscribed_status_mutex);
    mqttClientStatus.subscribed = FALSE;
    pthread_mutex_unlock(&g_subscribed_status_mutex);


    #ifndef USE_BUILD_IN_MQTT_RECONNECT
    Log_info("Reconnecting...");
    MQTTClientWrapper_connect();
    #endif
}

static void on_message_send(void* context, MQTTAsync_successData* response)
{
    Log_debug("on_message_send");
}

static void on_messasge_send_failure(void* context,  MQTTAsync_failureData* response)
{
    Log_error("on_messasge_send_failure");
}

void on_subscribe(void* context, MQTTAsync_successData* response)
{
    Log_debug("on_subscribe");

    pthread_mutex_lock(&g_subscribed_status_mutex);
    mqttClientStatus.subscribed = TRUE;
    pthread_mutex_unlock(&g_subscribed_status_mutex);
}

void on_subscribe_failure(void* context, MQTTAsync_failureData* response)
{
    Log_debug("on_subscribe_failure, rc %d", response ? response->code : 0);
    Log_error("MQTT subscribe failure: rc %d", response ? response->code : 0)

    pthread_mutex_lock(&g_subscribed_status_mutex);
    mqttClientStatus.subscribed = FALSE;
    pthread_mutex_unlock(&g_subscribed_status_mutex);
}

int message_arrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    Log_debug("message_arrived");
    Log_info("MQTT message arrived: topic: %s", topicName && topicLen ? topicName : "");

    if (mqttClientStatus.messageReceivedCallback
        && topicName && topicLen > 0
        && message && message->payload && message->payloadlen) {
        mqttClientStatus.messageReceivedCallback(topicName, (unsigned char*)message->payload, message->payloadlen);
    }

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);

    return TRUE;
}



/*************************************************************************************
 Helpers
**************************************************************************************/
#ifdef OPENSSL
static char g_sslCertFilePath[_POSIX_PATH_MAX] = { 0 };
static char *get_ssl_cert_file_path() {
    if (string_is_empty(g_sslCertFilePath)) {
        local_path(g_sslCertFilePath);
        strcat(g_sslCertFilePath, "/");
        strcat(g_sslCertFilePath, APP_DATA_PATH);
        strcat(g_sslCertFilePath, "/");
        strcat(g_sslCertFilePath, "mqtt.cert");
    }
    return g_sslCertFilePath;
}
static MQTTAsync_SSLOptions g_sslOptions = MQTTAsync_SSLOptions_initializer;
#endif

static void fill_connect_opts(MQTTAsync_connectOptions *conn_opts) 
{
    conn_opts->username = MQTT_USER;
    conn_opts->password = MQTT_PASSWD;
    conn_opts->keepAliveInterval = KEEPALIVEINTERVAL;
    conn_opts->cleansession = CLEANSESSION;
    conn_opts->connectTimeout = CONNECT_TIMEOUT;
    conn_opts->onSuccess = on_connect;
    conn_opts->onFailure = on_connect_failure;
    conn_opts->context = mqttClientStatus.client;

    #ifdef USE_BUILD_IN_MQTT_RECONNECT
    Log_info("Use build-in MQTT reconnect");
    conn_opts->automaticReconnect = TRUE;
    conn_opts->retryInterval = RETRY_INTERVAL;
    conn_opts->minRetryInterval = MIN_RETRY_INTERVAL;
    conn_opts->maxRetryInterval = MAX_RETRY_INTERVAL; 
    #endif

    #ifdef OPENSSL
    g_sslOptions.enableServerCertAuth = TRUE;
    g_sslOptions.trustStore = get_ssl_cert_file_path();
    conn_opts->ssl = &g_sslOptions;
    #endif
}

/*************************************************************************************
 Publics
**************************************************************************************/

/*
 Init mqtt client
*/
static char *get_client_id();
int MQTTClientWrapper_init()
{
    Log_debug("MQTTClientWrapper_init");

    int retCode = MQTTASYNC_FAILURE;

    if (mqttClientStatus.client) {
        Log_debug("Free (disconnected) MQTT client");
        MQTTAsync_destroy(&mqttClientStatus.client);
    }

    char *clientId = get_client_id();
    if (string_is_empty(clientId)) {
        Log_error("MQTT client id is empty");
        return FAILURE;
    }

    Log_debug("MQTT Server: %s clientId: %s", get_config_mqtt_end_point(), clientId);

    retCode = MQTTAsync_create(&mqttClientStatus.client, get_config_mqtt_end_point(), clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);

    if (retCode != MQTTASYNC_SUCCESS) {
        Log_error("MQTTAsync_create failed, error: %d", retCode);
        return retCode;
    }

    Log_info("Init MQTT client successfully");

    MQTTAsync_setCallbacks(mqttClientStatus.client, NULL, on_connection_lost, message_arrived, NULL);

    return SUCCESS;
}

/*
 connect mqtt client
*/
int MQTTClientWrapper_connect() 
{
    Log_debug("MQTTClientWrapper_connect");

    int retCode = MQTTASYNC_FAILURE;

    if (!mqttClientStatus.client) {
        Log_error("Client has not been initilaized, MQTTClientWrapper_init should be called");
        return FAILURE;
    }

    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    fill_connect_opts(&conn_opts);

    MQTTAsync_willOptions willOptions = MQTTAsync_willOptions_initializer;
    conn_opts.will = &willOptions;
    conn_opts.will->qos = MAXQOS;
    conn_opts.will->topicName = CAT_TOPIC(MAIN_TOPIC,OFFLINE_SUB_TOPIC);
    char willMessage[64] = { 0 };
    sprintf(willMessage, "{\"%s\":\"%s\"}", JSON_NODE_SN_KEY, get_device_sn());
    conn_opts.will->message = willMessage;

    if ((retCode = MQTTAsync_connect(mqttClientStatus.client, &conn_opts)) == MQTTASYNC_SUCCESS) {
        Log_info("Start connecting...");
        return SUCCESS;
    }
    else {
        Log_error("Failed to start connect, return code %d", retCode);

        pthread_mutex_lock(&g_connected_status_mutex);
        mqttClientStatus.connected = FALSE;
        pthread_mutex_unlock(&g_connected_status_mutex);

        return FAILURE;
    }
}

/*
 check connect status
*/
BOOL MQTTClientWrapper_isConnected()
{
    Log_debug("MQTTClientWrapper_isConnected");
    
    if (!mqttClientStatus.client) {
        Log_info("Connected: %d", 0);
        return FALSE;
    }

    BOOL connected = FALSE;
    pthread_mutex_lock(&g_connected_status_mutex);
    connected = mqttClientStatus.connected;
    pthread_mutex_unlock(&g_connected_status_mutex);
    return connected;

    // BOOL connected = MQTTAsync_isConnected(mqttClientStatus.client);
    // Log_info("Connected: %s", connected ? "True" : "False");
    // return connected;
}

/*
 check if MQTT has ever connected at least once
*/
BOOL MQTTClientWrapper_hasEverConnected()
{
    return mqttClientStatus.hasEverConnected;
}

/*
 check subscrible status
*/
BOOL MQTTClientWrapper_isSubscribed()
{
    BOOL subscribed = FALSE;

    pthread_mutex_lock(&g_subscribed_status_mutex);
    subscribed = mqttClientStatus.subscribed;
    pthread_mutex_unlock(&g_subscribed_status_mutex);

    return subscribed;
}

/*
 send mqtt message
*/
int MQTTClientWrapper_sendMessage(char *topic, char *message, int qos) {
    Log_debug("MQTTClientWrapper_sendMessage topic: %s message: %s qos: %d", topic, message, qos);

    MQTTAsync client = (MQTTAsync)mqttClientStatus.client;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    int rc = FAILURE;
    
    opts.onSuccess = on_message_send;
    opts.onFailure = on_messasge_send_failure;
    opts.context = client;

    /*
    Encrypt payload 
    */

#ifdef PAYLOAD_ENCRYPT
    mqtt_encrypted_payload_header header;
    char *encryptedPayload = NULL;
    unsigned long encryptedPayloadLen = 0;
    if (mqtt_payload_encrypt(message, strlen(message), &encryptedPayload, &encryptedPayloadLen, &header) != SUCCESS) {
        Log_error("Encrypt payload failed");
        if (encryptedPayload) {
            free(encryptedPayload);
        }
        return FAILURE;
    }

    Log_debug("encryptedPayloadLen: %d", encryptedPayloadLen);

    pubmsg.payload = encryptedPayload;
    pubmsg.payloadlen = encryptedPayloadLen;

#else
    pubmsg.payload = message;
    pubmsg.payloadlen = strlen(message);
#endif

    if (qos >= QOS0 && qos <= MAXQOS) {
        pubmsg.qos = qos;
    }
    else {
        Log_warn("QOS %d is not valid, use default: %d", qos, DEFAULT_SEND_MSG_QOS);
        pubmsg.qos = DEFAULT_SEND_MSG_QOS;
    }

    pubmsg.retained = 0;

    if ((rc = MQTTAsync_sendMessage(client, topic, &pubmsg, &opts)) == MQTTASYNC_SUCCESS) {
        increase_sent_msg_count();
        rc = SUCCESS;
    }
    else {
        Log_error("Failed to start send message, return code %d", rc);
        rc = FAILURE;
    }

#ifdef PAYLOAD_ENCRYPT
    if (encryptedPayload) {
        free(encryptedPayload);
    }
#endif

    return rc;
}


/*
 set arrived message callback
*/
void MQTTClientWrapper_setMessageReceivedCallback(MQTTClientWrapper_messageReceivedCallback *callback)
{
    Log_debug("MQTTClientWrapper_setMessageReceivedCallback");

    mqttClientStatus.messageReceivedCallback = callback;
}

/*
 set MQTT client connected callback function
*/
void MQTTClientWrapper_setConnectedCallback(MQTTClientWrapper_connectedCallback callback)
{
    Log_debug("MQTTClientWrapper_setConnectedCallback");

    mqttClientStatus.connectedCallback = callback;
}

/*
 subscrible
*/
int MQTTClientWrapper_subscribe()
{
    Log_debug("MQTTClientWrapper_subscribe");

    MQTTAsync client = (MQTTAsync)mqttClientStatus.client;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    int rc = FAILURE;

    opts.onSuccess = on_subscribe;
    opts.onFailure = on_subscribe_failure;
    opts.context = client;

    char *topics[2];
    char topic1[128];
    char topic2[128];
    int qos[2] = { DEFAULT_SUBSCRIBE_QOS, DEFAULT_SUBSCRIBE_QOS };

    memset(topic1, 0, sizeof(topic1));
    memset(topic2, 0, sizeof(topic2));

    strcpy(topic1, CAT_TOPIC(CAT_TOPIC(MAIN_TOPIC, CMD_SUB_TOPIC),CMD_ALL_SUB_TOPIC));
    strcpy(topic2, CAT_TOPIC(MAIN_TOPIC, CMD_SUB_TOPIC));
    strcat(topic2, MQTT_TOPIC_SPLITTER);
    strcat(topic2, get_device_sn());

    topics[0] = topic1;
    topics[1] = topic2;

    Log_debug("topic 0 to be subscribed: %s", topics[0]);
    Log_debug("topic 1 to be subscribed: %s", topics[1]);

    if ((rc = MQTTAsync_subscribeMany(client, 2, topics, qos, &opts)) != MQTTASYNC_SUCCESS)
    {
        Log_error("Failed to start subscribe, return code %d", rc);
        return FAILURE;
    }

    return SUCCESS;
}

/*
 disconnect mqtt client
*/
int MQTTClientWrapper_disconnect()
{
    Log_debug("MQTTClientWrapper_disconnect");

    MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
    opts.onSuccess = on_disconnect;
    opts.onFailure = on_disconnect_failure;

    int retCode = MQTTASYNC_FAILURE;

    if (MQTTClientWrapper_isConnected()) {
        retCode = MQTTAsync_disconnect(mqttClientStatus.client, &opts);
        if (retCode == MQTTASYNC_SUCCESS) {
            Log_info("Start disconnect...");
            return SUCCESS;
        }
        else {
            Log_error("Disconnect error: %d",retCode);
            return FAILURE;
        }
    }
    else {
        Log_warn("MQTT client is not connected");
        return SUCCESS;
    }
}

/*
 free mqtt client
*/
void MQTTClientWrapper_deinit()
{
    Log_debug("MQTTClientWrapper_deinit");

    if (mqttClientStatus.client) {
        Log_debug("Free MQTT client");

        MQTTAsync_destroy(&mqttClientStatus.client);
        mqttClientStatus.client = NULL;
    }
    else {
        Log_warn("MQTT client object is NULL, maybe it has been freed or has not initlized");
    }
}

/*
 error msssage from error code
*/
static char g_mqttErrorMessage[8] = { 0 };
char *MQTTClientWrapper_errorDescription(int mqttErrorCode)
{
    if (mqttErrorCode == MQTTASYNC_SUCCESS) {
        return "SUCCESS";
    }
    else if (mqttErrorCode == MQTTASYNC_FAILURE) {
        return "FAILURE";
    }
    else if (mqttErrorCode == MQTTASYNC_PERSISTENCE_ERROR) {
        return "PERSISTENCE_ERROR";
    }
    else if (mqttErrorCode == MQTTASYNC_DISCONNECTED) {
        return "DISCONNECTED";
    }
    else if (mqttErrorCode == MQTTASYNC_MAX_MESSAGES_INFLIGHT) {
        return "MAX_MESSAGES_INFLIGHT";
    }
    else if (mqttErrorCode == MQTTASYNC_BAD_UTF8_STRING) {
        return "BAD_UTF8_STRING";
    }
    else if (mqttErrorCode == MQTTASYNC_NULL_PARAMETER) {
        return "NULL_PARAMETER";
    }
    else if (mqttErrorCode == MQTTASYNC_BAD_STRUCTURE) {
        return "BAD_STRUCTURE";
    }
    else if (mqttErrorCode == MQTTASYNC_BAD_QOS) {
        return "BAD_QOS";
    }
    else if (mqttErrorCode == MQTTASYNC_NO_MORE_MSGIDS) {
        return "NO_MORE_MSGIDS";
    }
    else if (mqttErrorCode == MQTTASYNC_OPERATION_INCOMPLETE) {
        return "OPERATION_INCOMPLETE";
    }
    else if (mqttErrorCode == MQTTASYNC_MAX_BUFFERED_MESSAGES) {
        return "MAX_BUFFERED_MESSAGES";
    }

    sprintf(mqttErrorCode,"%d", mqttErrorCode);
    Log_debug("%d",mqttErrorCode);
    return mqttErrorCode;
}

/*************************************************************************************
 Helpers
**************************************************************************************/
static char g_ClientId[48] = { 0 };
static char *get_client_id()
{
    if (string_is_empty(g_ClientId)) {
        char *sn = get_device_sn();
        if (!string_is_empty(sn)) {
            strcpy(g_ClientId, CLIENTIDPREFIX);
            strcat(g_ClientId, sn);
        }

        Log_info("MQTT client id: %s", g_ClientId);
    }
    return g_ClientId;
}

