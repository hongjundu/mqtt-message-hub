/*
 *  File: httpwrapper.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Sun Peijian on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <errno.h>
#include <sys/stat.h> 
#include "httpwrapper.h"

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    Resp* resp = (Resp*) userp;
    size_t realsize = size * nmemb;
    size_t copysize = 0;
    // printf("into write_callback, userp:%p\n", userp);
    if(realsize > MAX_RESPONSE_SIZE - resp->ret_size) {
        copysize = MAX_RESPONSE_SIZE - resp->ret_size;

    } else {
        copysize = realsize;
    }
    // printf("recvsize:%ld, copysize:%ld, ret_size:%ld, real_size:%ld\n", realsize, copysize, resp->ret_size, resp->real_size);
    if(copysize) {
        memcpy(&(resp->content[resp->ret_size]), contents, copysize);
        resp->ret_size += copysize;
    }
    resp->real_size += realsize;
    // printf("recvsize:%ld, copysize:%ld, ret_size:%ld, real_size:%ld\n", realsize, copysize, resp->ret_size, resp->real_size);
    
    return realsize;
}

/*****************************************************
* http_init()
* return: HTTP_WRAPPER_STATUS_OK/HTTP_WRAPPER_STATUS_ERROR
******************************************************/
int http_init() {
    /* In windows, this will init the winsock stuff */ 
    curl_global_init(CURL_GLOBAL_ALL);
    return HTTP_WRAPPER_STATUS_OK;
}

/*****************************************************
* http_cleanup()
******************************************************/
void http_cleanup() {
    curl_global_cleanup();
}


/*****************************************************
* http_get()
* param: url, timeout
* return: Resp struct
******************************************************/
Resp* http_get(char* url, int timeout) {
    CURLcode res;
    Resp* resp = (Resp*) malloc(sizeof(Resp));
    memset(resp, 0, sizeof(Resp));

    CURL* curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)resp);
        if (timeout > 0) {
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        }

        res = curl_easy_perform(curl);
        resp->curl_code = res;
        printf("curl code: %d\n", res);
        if(res != CURLE_OK) {
          // fprintf(stderr, "curl_easy_perform() failed: %s\n",
          //         curl_easy_strerror(res));
        }
        
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &(resp->status_code));
        curl_easy_cleanup(curl);
    }
    return resp;
}

/*****************************************************
* http_post()
* param: url, post_fields, timeout
* return: Resp struct
******************************************************/
Resp* http_post(char* url, char* post_fields, int timeout) {
    CURLcode res;
    Resp* resp = (Resp*) malloc(sizeof(Resp));
    memset(resp, 0, sizeof(Resp));

    CURL* curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)resp);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);
        if (timeout > 0) {
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        }

        res = curl_easy_perform(curl);
        resp->curl_code = res;
        printf("curl code: %d\n", res);
        if(res != CURLE_OK) {
          // fprintf(stderr, "curl_easy_perform() failed: %s\n",
          //         curl_easy_strerror(res));
        }
        
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &(resp->status_code));
        curl_easy_cleanup(curl);
    }
    return resp;
}

/*****************************************************
* http_post_json()
* param: url, json_str, timeout
* return: Resp struct
******************************************************/
Resp* http_post_json(char* url, char* json_str, int timeout) {
    printf("http_post_json\n");

    CURLcode res;
    Resp* resp = (Resp*) malloc(sizeof(Resp));
    memset(resp, 0, sizeof(Resp));

    CURL* curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)resp);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);
        struct curl_slist *plist = curl_slist_append(NULL, "Content-Type:application/json;charset=UTF-8");  
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, plist);
        if (timeout > 0) {
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        }

        res = curl_easy_perform(curl);
        resp->curl_code = res;
        printf("curl code: %d\n", res);
        if(res != CURLE_OK) {
            // fprintf(stderr, "curl_easy_perform() failed: %s\n",
            //         curl_easy_strerror(res));
        }
        
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &(resp->status_code));
        curl_slist_free_all(plist);
        curl_easy_cleanup(curl);
    }
    return resp;
}

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    curl_off_t nread;
    /* in real-world cases, this would probably get this data differently
        as this fread() stuff is exactly what the library already would do
        by default internally */ 
    size_t retcode = fread(ptr, size, nmemb, stream);
 
    nread = (curl_off_t)retcode;
 
    //fprintf(stderr, "*** We read %" CURL_FORMAT_CURL_OFF_T
    //      " bytes from file\n", nread);
    
    return retcode;
}

int ftp_upload(char* url, char *filename)
{

    printf("ftp_upload %s\n", filename);
    CURL *curl;
    CURLcode res;
    FILE *hd_src;
    struct stat file_info;
    curl_off_t fsize;
    int rc = 0;
    char ftpFile[1024] = { 0 };


    /* get the file size of the local file */ 
    if(stat(filename, &file_info)) {
        printf("Couldnt open '%s': %s\n", filename, strerror(errno));
        return 1;
    }
    fsize = (curl_off_t)file_info.st_size;
 
    printf("Local file size: %" CURL_FORMAT_CURL_OFF_T " bytes.\n", fsize);
 
    /* get a FILE * of the same file */ 
    hd_src = fopen(filename, "rb");
 
    /* get a curl handle */ 
    curl = curl_easy_init();
    if(curl) {
        sprintf(ftpFile, "%s/%s", url, filename);
        /* we want to use our own read function */ 
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
 
        /* enable uploading */ 
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
 
        /* specify target */ 
        curl_easy_setopt(curl, CURLOPT_URL, ftpFile);
        curl_easy_setopt(curl, CURLOPT_USERPWD, "nocftpupload:tCZn5AKoY3pMA");

        /* now specify which file to upload */ 
        curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);
 
        /* Set the size of the file to upload (optional).  If you give a *_LARGE
            option you MUST make sure that the type of the passed-in argument is a
            curl_off_t. If you use CURLOPT_INFILESIZE (without _LARGE) you must
            make sure that to pass in a type 'long' argument. */ 
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fsize);
 
        /* Now run off and do what you've been told! */ 
        res = curl_easy_perform(curl);
        /* Check for errors */ 
        if(res != CURLE_OK)
        {
            //fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            rc = 1;
        }
        /* always cleanup */ 
        curl_easy_cleanup(curl);
    }
    fclose(hd_src); /* close the local file */ 
 
    return rc;
}

/*****************************************************
* status_code_to_string()
* param: code
* return: string
******************************************************/
char *status_code_to_string(int code)
{
    switch(code)
    {
    case HTTP_STATUS_CODE_OK:
        return "OK";
    case HTTP_STATUS_CODE_BAD_REQUEST:
        return "Bad Request";
    case HTTP_STATUS_CODE_UNAUTHORIZED:
        return "Unauthorized";
    case HTTP_STATUS_CODE_FORBIDDEN:
        return "Forbidden";
    case HTTP_STATUS_CODE_NOT_FOUND:
        return "Not Found";
    case HTTP_STATUS_CODE_METHOD_NOT_ALLOWED:
        return "Method Not Allowed";
    case HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR:
        return "Internal Server Error";
    case HTTP_STATUS_CODE_BAD_GATEWAY:
        return "Bad Gateway";
    case HTTP_STATUS_CODE_SERVICE_UNAVAILABLE:
        return "Service Unavailable";
    case HTTP_STATUS_CODE_GATEWAY_TIMEOUT:
        return "Gateway Timeout";
    default:
        break;
    }
    return "Error";
}
