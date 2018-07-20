/*
 *  File: stringutils.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#pragma once
#include <stdio.h>
#include <string.h>

/*
 check string is NULL or empty
*/
int string_is_empty(const char *s); 

/*
 check if is number string
*/
int string_is_number(char *str);

/*
 comare two strings
*/
int string_equals(const char *s1, const char *s2, int ignoreCase);

/*
 return empty string if NULL
*/
char *safe_string(char *str);

/*
 split string with delimiter
*/
int string_split(const char *str, char *parts[], const char *delimiter);


/* 
 trim left
*/
char *string_trim_left(char* str, char trim);

/* 
 trim right
*/
char *string_trim_right(char* str, char trim);

/* 
 trim left & right
*/
char *string_trim(char* str, char trim);

/*
 replace chars that are invalid for JSON
*/
void string_json_friendly(char *str);

/*
 convert string to upper case
*/
void string_upper_case(char *str);

/*
 convert string to lower case
*/
void string_lower_case(char *str);

/*
 replace a char with a string,
 newstr MUST be freed by caller
*/
void string_replace_char(char *str, char c, char *replace, char **newstr);