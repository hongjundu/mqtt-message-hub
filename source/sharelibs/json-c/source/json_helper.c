/*
 *  File: json_helper.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/07.
 *  Copyright 2018 All rights reserved.
 */

#include "json_helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int json_helper_get_string(json_object *jobj, char *key, char **str, char *errorMsg)
{
    json_object *jsubobj = 0;

    if (json_object_object_get_ex(jobj, key, &jsubobj)) {
        if (json_object_is_type(jsubobj, json_type_string)) {
            *str = (char*)json_object_get_string(jsubobj);
            return JSON_HELPER_OK;
        }
        else {
            sprintf(errorMsg, "node '%s' is not a string", key);
            return JSON_HELPER_ERROR_TYPE_NOT_MATCH;
        }
    }

    sprintf(errorMsg, "no node '%s'", key);
    
    return JSON_HELPER_ERROR_NO_NODE;
}

int json_helper_get_object(json_object *jobj, char *key, json_object **subobj, char *errorMsg)
{
    json_object *jsubobj = 0;

    if (json_object_object_get_ex(jobj, key, &jsubobj)) {
        if (json_object_is_type(jsubobj, json_type_object)) {
            *subobj = jsubobj;
            return JSON_HELPER_OK;
        }
        else {
            sprintf(errorMsg, "node '%s' is not a JSON object", key);
            return JSON_HELPER_ERROR_TYPE_NOT_MATCH;
        }
    }

    sprintf(errorMsg, "no node '%s'", key);
    
    return JSON_HELPER_ERROR_NO_NODE;
}

int json_helper_get_int64(json_object *jobj, char *key, int64_t *value, char *errorMsg)
{
    json_object *jsubobj = 0;

    if (json_object_object_get_ex(jobj, key, &jsubobj)) {
        if (json_object_is_type(jsubobj, json_type_int)) {
            *value = json_object_get_int64(jsubobj);
            return JSON_HELPER_OK;
        }
        else {
            sprintf(errorMsg, "node '%s' is not a int64", key);
            return JSON_HELPER_ERROR_TYPE_NOT_MATCH;
        }
    }

    sprintf(errorMsg, "no node '%s'", key);
    
    return JSON_HELPER_ERROR_NO_NODE;
}

int json_helper_get_int(json_object *jobj, char *key, int *value, char *errorMsg)
{
    json_object *jsubobj = 0;

    if (json_object_object_get_ex(jobj, key, &jsubobj)) {
        if (json_object_is_type(jsubobj, json_type_int)) {
            *value = json_object_get_int(jsubobj);
            return JSON_HELPER_OK;
        }
        else {
            sprintf(errorMsg, "node '%s' is not a int", key);
            return JSON_HELPER_ERROR_TYPE_NOT_MATCH;
        }
    }
    
    sprintf(errorMsg, "no node '%s'", key);

    return JSON_HELPER_ERROR_NO_NODE;
}

int json_helper_get_double(json_object *jobj, char *key, double *value, char *errorMsg)
{
    json_object *jsubobj = 0;

    if (json_object_object_get_ex(jobj, key, &jsubobj)) {
        if (json_object_is_type(jsubobj, json_type_double)) {
            *value = json_object_get_double(jsubobj);
            return JSON_HELPER_OK;
        }
        else {
            sprintf(errorMsg, "node '%s' is not a double", key);
            return JSON_HELPER_ERROR_TYPE_NOT_MATCH;
        }
    }
    
    sprintf(errorMsg, "no node '%s'", key);

    return JSON_HELPER_ERROR_NO_NODE;
}