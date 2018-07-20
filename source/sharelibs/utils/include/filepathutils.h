/*
 *  File: filepathutils.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#pragma once

#include <stdio.h>
#include <sys/types.h>
#include <time.h>

/*
 get local executable path
*/
void local_path(char *path);

/*
 get home path
*/
char *home_path();

/*
 * Recursively `mkdir(path, mode)`
 */

int mkdirp(const char *, mode_t );

/*
 check if path exits
*/
int path_exists(const char *path);

/*
 check if file exits
*/
int file_exists(const char *path);

/*
 return 1 if path is a file, 0 for directory
*/
int is_file(const char *path);

/*
 return 1 if path is a directory, 0 for file
*/
int is_dir(const char *path);

/*
 get a parent dir of a file or dir
*/
void get_parent_dir(char *dirOrFile, /*out*/char *outParentDir);

/*
 get file suffix
*/
void get_file_suffix(const char *path, char *suffixBuffer);

/*
 get file base name, suffix removed
*/
void get_file_base_name(const char *path, char *baseNameBuffer);

/*
 get dir/file name, with suffix
*/
void get_name_from_path(const char *path, char *nameBuff);

/*
 write string to file, returns write size
*/
size_t write_file_string(const void * ptr, size_t size, size_t count, const char *filePath, const char *mode);

/*
read string to buffer, caller must free the buffer, returns read size
*/
size_t read_file_string(char **buffer,const char *filePath);

/*
 get full file path in app path
*/
int get_local_file_path(char *fileName, char *filePath);

/*
 get full dir path in app path
*/
int get_local_dir_path(char *dirName, char *filePath);

/*
 get last modified time of a file
*/
time_t get_file_modified_time(const char *path);


/*
 write application run version file to /tmp/appName-ver-runtime
*/
int write_app_run_version_file(const char *appName, const char *version);

/*
 delete application run version file: /tmp/appName-ver-runtime
*/
void delete_app_run_version_file(const char *appName);
