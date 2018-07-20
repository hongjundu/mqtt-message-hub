/*
 *  File: ini_parser_helper.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#include "ini_parser_helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "stringutils.h"
#include "filepathutils.h"
#include "ini_parser.h"

void *create_app_ini_parser(char *iniFileName)
{
    char iniPath[_POSIX_PATH_MAX];
    int rc = get_local_file_path(iniFileName, iniPath);
    if (rc < 0) {
        printf("get ini path failed, [%d] %s\n", errno,strerror(errno));
        return 0;
    }

    struct ini_parser *parser = new_ini_parser();
    if (!parser) {
        printf("%s", "create ini parser failed\n");
        return 0;
    }

    int size = parser->parse_file(parser,iniPath);
    if (size <= 0) {
        printf("Nothing was read from ini file: %s\n", iniPath);
        parser->deletor(parser);
        return 0;
    }

    return parser;
}

int get_int_from_ini_file(char *iniFileName, char *key, int defaultValue)
{
    printf("get_int_from_ini_file\n");

    int value = defaultValue;

    struct ini_parser *parser = (struct ini_parser*)create_app_ini_parser(iniFileName);
    if (!parser) {
        return value;
    }

    char *str = parser->value(parser, key);
    if (string_is_empty(str)) {
        printf("No '%s' was congigured\n", key);
    }
    else {
        if (string_is_number(str)) {
            value = atoi(str);
            printf("%d\n", value);
        }
        else {
            printf("%s is not a valid number format\n", str);
        }
    }

    parser->deletor(parser);

    return value;
}

int get_string_from_ini_file(char *iniFileName, char *key, char *defaultValue, char *value)
{
    printf("get_string_from_ini_file\n");

    if (defaultValue) {
        strcpy(value, defaultValue);
    }

    struct ini_parser *parser = (struct ini_parser*)create_app_ini_parser(iniFileName);
    if (!parser) {
        return -1;
    }

    char *str = parser->value(parser, key);
    if (string_is_empty(str)) {
        printf("No '%s' was congigured\n", key);
        parser->deletor(parser);
        return -1;
    }
    else {
        printf("%s\n", str);
        strcpy(value, str);
    }

    parser->deletor(parser);

    return 0;
}
