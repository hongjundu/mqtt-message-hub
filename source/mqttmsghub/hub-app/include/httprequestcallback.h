/*
 *  File: httprequestcallback.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#pragma once
#include <stdlib.h>
#include <stdio.h>
#include "httpd.h"

void http_callback_mqtt_send_discard_failed(httpd *webserver, request *r);

void http_callback_mqtt_send_cache_failed(httpd *webserver, request *r);

void http_callback_mqtt_status(httpd *webserver, request *r);

void http_callback_404(httpd *webserver, request *r);