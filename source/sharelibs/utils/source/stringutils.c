/*
 *  File: stringutils.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#include "stringutils.h"
#include <stdlib.h>
#include <string.h>
//#include <strdup.h>
/*
 check string is NULL or empty
*/
int string_is_empty(const char *s)
{
    return (!s || strlen(s) == 0);
}

/*
 check if is number string
*/
int string_is_number(char *str)
{
    if (string_is_empty(str)) {
      return 0;
    }

    while(*str){
        if (*str < '0' || *str > '9') {
            return 0;
        }
        str++;
    }
    return 1;
}

/*
 comare two strings
*/
int string_equals(const char *s1, const char *s2, int ignoreCase)
{
    int (*cmp_fn)(const char*,const char*) = strcmp;

    if (ignoreCase) {
        cmp_fn = strcasecmp;
    }
    return cmp_fn(s1, s2) == 0;
}

/*
 return empty string if NULL
*/
static char *EMPTY_STRING = "";
char *safe_string(char *str)
{
    if (!str) {
      return EMPTY_STRING;
    }
    return str;
}

/*
 split string with delimiter
*/
int string_split (const char *str, char *parts[], const char *delimiter) {
  char *pch;
  int i = 0;
  char *copy = NULL, *tmp = NULL;

  copy = strdup(str);
  if (! copy)
    goto bad;

  pch = strtok(copy, delimiter);

  tmp = strdup(pch);
  if (! tmp)
    goto bad;

  parts[i++] = tmp;

  while (pch) {
    pch = strtok(NULL, delimiter);
    if (NULL == pch) break;

    tmp = strdup(pch);
    if (! tmp)
      goto bad;

    parts[i++] = tmp;
  }

  free(copy);
  return i;

 bad:
  free(copy);
  int j = 0;
  for (; j < i; j++)
    free(parts[j]);
  return -1;
}


/* 
 trim left
*/
char *string_trim_left(char* str, char trim)
{
    // Trim leading
  while(*str == trim) str++;
  return str;
}

/* 
 trim right
*/
char *string_trim_right(char* str, char trim)
{
  char *end;

  // Trim trailing
  end = str + strlen(str) - 1;
  while(end > str && *end == trim) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

/* 
 trim left & right
*/
char* string_trim(char* str, char trim)
{
  char *end;

  // Trim leading
  while(*str == trim) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing
  end = str + strlen(str) - 1;
  while(end > str && *end == trim) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

/*
 replace chars that are invalid for JSON
*/

typedef struct {
    char replaceChar;
    char *withString;
} json_replace_char;

static json_replace_char g_jsonReplaceChars[] = 
{
    {'\\', "\\\\"},
    {'/', "\\/"},
    {'\b', "\\b"},
    {'\f', "\\f"},
    {'\n', "\\n"},
    {'\r', "\\r"},
    {'\t', "\\t"},
    {'\'', "\\\'"},
    {'\"', "\\\""}
};

static json_replace_char* find_json_replace_char(char c) 
{
    int count = sizeof(g_jsonReplaceChars) / sizeof(g_jsonReplaceChars[0]);
    int i = 0;
    for(; i < count; i++) {
        if (g_jsonReplaceChars[i].replaceChar == c) {
            return &g_jsonReplaceChars[i];
        }
    }
    return 0;
}

void string_json_friendly(char *str)
{
    char *newstr = (char*)malloc(strlen(str) * 2);

    char *p = str;
    char *q = newstr;

    while(*p) {
        json_replace_char *jrc = find_json_replace_char(*p);
        if (jrc) {
            char *replace = jrc->withString;
            while(*replace) {
                *q++ = *replace++;
            }
            p++;
        }
        else {
            *q++ = *p++;
        }
    }

    *q = 0;

    strcpy(str,newstr);

    free(newstr);
}

/*
 convert string to upper case
*/
void string_upper_case(char *str)
{
    while(*str) {
        if ((*str >= 'a') && (*str <= 'z')) {
            *str -= 32;
        }
        str++;
    }
}

/*
 convert string to lower case
*/
void string_lower_case(char *str)
{
    while(*str) {
        if ((*str >= 'A') && (*str <= 'Z')) {
            *str += 32;
        }
        str++;
    }
}

/*
 replace a char with a string
*/
void string_replace_char(char *str, char c, char *replace, char **newstr)
{
    int count = 0;
    char *p = str;

    while (*p) {
        if (*p == c) {
            count++;
        }
        p++;
    }

    if (count == 0) {
        return;
    }

    count = strlen(str) - count + count * strlen(replace) + 1;

    *newstr = (char*)malloc(count);

    p = str;
    char *q = *newstr;

    while(*p) {
        if (*p == c) {
            char *r = replace;
            while (*r) {
                *q++ = *r++;
            }
            p++;
        }
        else {
            *q++ = *p++;
        }
    }

    *q = 0;
}

