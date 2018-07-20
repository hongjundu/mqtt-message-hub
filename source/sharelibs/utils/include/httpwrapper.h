/*
 *  File: httpwrapper.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Sun Peijian on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#ifndef __HTTP_WRAPPER_H_ /* include guard */
#define __HTTP_WRAPPER_H_

#include <stdlib.h>

#define MAX_RESPONSE_SIZE 4096
#define HTTP_WRAPPER_STATUS_OK 0
#define HTTP_WRAPPER_STATUS_ERROR -1

/*
 common http status code
 TODO: add more for your needs
*/
typedef enum 
{
    HTTP_STATUS_CODE_OK = 200,
    
    HTTP_STATUS_CODE_BAD_REQUEST = 400,
    HTTP_STATUS_CODE_UNAUTHORIZED = 401,
    HTTP_STATUS_CODE_FORBIDDEN = 403,
    HTTP_STATUS_CODE_NOT_FOUND = 404,
    HTTP_STATUS_CODE_METHOD_NOT_ALLOWED = 405,
    HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR = 500,
    HTTP_STATUS_CODE_BAD_GATEWAY = 502,
    HTTP_STATUS_CODE_SERVICE_UNAVAILABLE = 503,
    HTTP_STATUS_CODE_GATEWAY_TIMEOUT = 504
}HttpStatusCode;

#define HTTP_STATUS_OK(code)    (code == HTTP_STATUS_CODE_OK)
#define HTTP_STATUS_ERROR(code) (code != HTTP_STATUS_CODE_OK)

typedef struct
{
    char content[MAX_RESPONSE_SIZE + 1];   /* conent buffer */
    long curl_code;                        /* curl code */
    long status_code;                      /* http status */
    size_t ret_size;                       /* returned content size */
    size_t real_size;                      /* actual content size */
} Resp;

int http_init();
void http_cleanup();
Resp* http_get(char* url, int timeout);
Resp* http_post(char* url, char* post_fields, int timeout);
Resp* http_post_json(char* url, char* json_str, int timeout);
int ftp_upload(char* url, char *filename);
char *status_code_to_string(int code);

#endif /* __HTTP_WRAPPER_H_ */