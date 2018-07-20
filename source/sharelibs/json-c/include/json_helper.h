/*
 *  File: json_helper.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/07.
 *  Copyright 2018 All rights reserved.
 */

#ifndef _json_helper_h_
#define _json_helper_h_

#include <sys/types.h> 
#include "json.h"

#define JSON_HELPER_OK                   0
#define JSON_HELPER_ERROR                -1
#define JSON_HELPER_ERROR_NO_NODE        -2
#define JSON_HELPER_ERROR_TYPE_NOT_MATCH -3

int json_helper_get_string(json_object *jobj, char *key, char **str, char *errorMsg);

int json_helper_get_object(json_object *jobj, char *key, json_object **subobj, char *errorMsg);

int json_helper_get_int64(json_object *jobj, char *key, int64_t *value, char *errorMsg);

int json_helper_get_int(json_object *jobj, char *key, int *value, char *errorMsg);

int json_helper_get_double(json_object *jobj, char *key, double *value, char *errorMsg);

#endif