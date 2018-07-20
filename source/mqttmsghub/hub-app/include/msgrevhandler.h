/*
 *  File: msgrevhandler.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#pragma once
#include "common.h"

BOOL msg_received_handler(char *topic, unsigned char *payload, int payloadLen);