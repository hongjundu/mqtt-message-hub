/*
 *  File: mqttclientwrapper.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#pragma once

#include "common.h"

/*
 Wrapper functions for mqtt-c client
*/

typedef BOOL MQTTClientWrapper_messageReceivedCallback(char *topic, unsigned char *payload, int payloadLen);
typedef void MQTTClientWrapper_connectedCallback();

/*
 Init mqtt client
*/
int MQTTClientWrapper_init();

/*
 connect mqtt client
*/
int MQTTClientWrapper_connect();

/*
 check connect status
*/
BOOL MQTTClientWrapper_isConnected();


/*
 check if MQTT has ever connected at least once
*/
BOOL MQTTClientWrapper_hasEverConnected();

/*
 check subscrible status
*/
BOOL MQTTClientWrapper_isSubscribed();

/*
 send mqtt message
*/
int MQTTClientWrapper_sendMessage(char *topic, char *message, int qos);

/*
 set arrived message callback
*/
void MQTTClientWrapper_setMessageReceivedCallback(MQTTClientWrapper_messageReceivedCallback *callback);

/*
 set MQTT client connected callback function
*/
void MQTTClientWrapper_setConnectedCallback(MQTTClientWrapper_connectedCallback callback);

/*
 subscrible
*/
int MQTTClientWrapper_subscribe();

/*
 disconnect mqtt client
*/
int MQTTClientWrapper_disconnect();

/*
 free mqtt client
*/
void MQTTClientWrapper_deinit();

/*
 error msssage from error code
*/
char *MQTTClientWrapper_errorDescription(int mqttErrorCode);
