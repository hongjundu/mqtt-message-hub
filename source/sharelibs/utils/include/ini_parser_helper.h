/*
 *  File: ini_parser_helper.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#pragma once

void *create_app_ini_parser(char *iniFileName);

int get_int_from_ini_file(char *iniFileName, char *key, int defaultValue);

int get_string_from_ini_file(char *iniFileName, char *key, char *defaultValue, char *value);