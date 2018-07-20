/*
 *  File: filepathutils.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#include "filepathutils.h"
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "stringutils.h"

#ifdef _WIN32
#define PATH_SEPARATOR   '\\'
#else
#define PATH_SEPARATOR   '/'
#endif

/*
 get local executable path
*/
void local_path(char *path)
{   
    pid_t mypid = getpid();
    char procPath[_POSIX_PATH_MAX];
    char distPath[_POSIX_PATH_MAX];

    memset(procPath, 0, sizeof(procPath));
    memset(distPath, 0, sizeof(distPath));

    sprintf(procPath, "/proc/%d/exe", mypid);
    int len = readlink(procPath, distPath, _POSIX_PATH_MAX);

    int i = len - 1;
    while(distPath[i] != '/' && i > 0) i--;

    distPath[i] = 0;

    strcpy(path, distPath);
}

/*
 get home path
*/
static char gHomePath[_POSIX_PATH_MAX] = { 0 };
char *home_path()
{
    if (strlen(gHomePath) == 0) {
        struct passwd *pwd = getpwuid(getuid());
        strcpy(gHomePath, pwd->pw_dir);
    }
    return gHomePath;
}

/*
 * Recursively `mkdir(path, mode)`
 */
static char *path_normalize(const char *path);
int mkdirp(const char *path, mode_t mode)
{
    char *pathname = NULL;
    char *parent = NULL;

    if (NULL == path) return -1;

    pathname = path_normalize(path);
    if (NULL == pathname) goto fail;

    parent = strdup(pathname);
    if (NULL == parent) goto fail;

    char *p = parent + strlen(parent);
    while (PATH_SEPARATOR != *p && p != parent) {
        p--;
    }
    *p = '\0';

    // make parent dir
    if (p != parent && 0 != mkdirp(parent, mode)) goto fail;
    free(parent);

    // make this one if parent has been made
    #ifdef _WIN32
    // http://msdn.microsoft.com/en-us/library/2fkk4dzw.aspx
    int rc = mkdir(pathname);
    #else
    int rc = mkdir(pathname, mode);
    #endif

    free(pathname);

    return 0 == rc || EEXIST == errno ? 0 : -1;

    fail:
    free(pathname);
    free(parent);
    return -1;
}

/*
 check if path exits
*/
int path_exists(const char *path)
{
    struct stat b;
    return stat(path, &b) == 0 ? 1 : 0;
}

/*
 check if file exits
*/
int file_exists(const char *path)
{
    return access(path, F_OK) == 0;
}

/*
 return 1 if path is a file, 0 for directory
*/
int is_file(const char *path)
{
    struct stat path_stat;
    if (stat(path, &path_stat) < 0) {
        return 0;
    }
    return S_ISREG(path_stat.st_mode);
}

/*
 return 1 if path is a directory, 0 for file
*/
int is_dir(const char *path)
{
    struct stat path_stat;
    if (stat(path, &path_stat) < 0) {
       return 0;
    }
    return S_ISDIR(path_stat.st_mode);
}

/*
 get a parent dir of a file or dir
*/
void get_parent_dir(char *dirOrFile, /*out*/char *outParentDir)
{
    char path[_POSIX_PATH_MAX] = { 0 };
    strcpy(path, dirOrFile);

    char *parentDir = string_trim(path, ' ');
    parentDir = string_trim_right(parentDir, '/');

    char *c = strrchr(parentDir, '/');
    if (c) {
        *c = 0;
    }

    if (parentDir) {
        strcpy(outParentDir, parentDir);
    }
}

/*
 get file suffix
*/
void get_file_suffix(const char *path, char *suffixBuffer)
{
    *suffixBuffer = 0;

    if (!path || strlen(path) == 0) {
        return;
    }

    char *dot = strrchr(path, '.');

    if (dot) {
        strcpy(suffixBuffer, dot);
    }
}

/*
 get file base name, suffix removed
*/
void get_file_base_name(const char *path, char *baseNameBuffer)
{
    *baseNameBuffer = 0;

    if (!path || strlen(path) == 0) {
        return;
    }

    char *dot = strrchr(path, '.');
    if (dot) {
        while(path != dot) {
            *baseNameBuffer++ = *path++;
        }
        *baseNameBuffer = 0;
    }
}

/*
 get dir/file name, with suffix
*/
void get_name_from_path(const char *path, char *nameBuff) 
{
    char *p = strrchr(path, '/');
    if (p) {
        p++;
    }
    if (p) {
        strcpy(nameBuff, p);
    }
}


/*
 write string to file, returns write size
*/
size_t write_file_string(const void *ptr, size_t size, size_t count, const char *filePath, const char *mode)
{
    size_t writeSize = 0;

    FILE *f = fopen(filePath,mode);
    if (!f) {
        return -1;
    }

    writeSize = fwrite(ptr, size,  count, f);
    fclose(f);

    return writeSize;
}

/*
read string to buffer, caller must free the buffer, returns read size
*/

size_t read_file_string(char **buffer,const char *filePath)
{
    size_t len = 0;
    *buffer = 0;

    FILE *f = fopen (filePath, "r");
    if (!f) {
        return -1;
    }

    fseek (f, 0, SEEK_END);
    len = ftell(f);
    fseek (f, 0, SEEK_SET);

    if (len > 0) {
        *buffer = (char*)malloc(len + 1);
        if (*buffer){
          fread (*buffer, sizeof(char), len, f);
        }
        *(*buffer + len) = 0;
    }
    fclose (f);

    return len;
}

/*
 get last modified time of a file
*/
time_t get_file_modified_time(const char *path)
{
    struct stat statbuf;
    if (stat(path, &statbuf) < 0) {
        printf("Error when get file status: %s\n", path);
        return 0;
    }
    return statbuf.st_mtime;
}

/*
 get full file path in app path
*/
int get_local_file_path(char *fileName, char *filePath)
{
    char path[_POSIX_PATH_MAX];

    memset(path, 0 ,sizeof(path));

    local_path(path);
    strcat(path,"/");
    strcat(path, (char*)(fileName));

    strcpy(filePath, path);

    if (access(path, F_OK) != 0) {
        printf("local file '%s' doesn't exit\n", filePath);
        return -1;
    }

    return 0;
}

/*
 get full dir path in app path
*/
int get_local_dir_path(char *dirName, char *filePath)
{
    char path[_POSIX_PATH_MAX] = { 0 };
    memset(path, 0 ,sizeof(path));

    local_path(path);
    strcat(path,"/");
    strcat(path, (char*)(dirName));

    strcpy(filePath, path);
    
    return 0;
}

/*
 write application run version file to /tmp/appName-ver-runtime
*/
int write_app_run_version_file(const char *appName, const char *version)
{
    char filePath[_POSIX_PATH_MAX] = { 0 };
    memset(filePath, 0 ,sizeof(filePath));

    sprintf(filePath, "/tmp/%s-ver-runtime", appName);

    size_t writeSize = write_file_string(version, sizeof(char), strlen(version), filePath, "w");

    if (writeSize <= 0) {
        return -1;
    }

    return 0;
}

/*
 delete application run version file: /tmp/appName-ver-runtime
*/
void delete_app_run_version_file(const char *appName)
{
    char filePath[_POSIX_PATH_MAX] = { 0 };
    sprintf(filePath, "/tmp/%s-ver-runtime", appName);
    unlink(filePath);
}

/*************************************************************************************
 Privates
**************************************************************************************/
static char *_strdup(const char *str) 
{
  int len = strlen(str) + 1;
  char *buf = malloc(len);
  if (buf) memcpy(buf, str, len);
  return buf;
}

static char *path_normalize(const char *path) {
  int i = 0;

  if (!path) return NULL;

  char *copy = _strdup(path);
  if (NULL == copy) return NULL;
  char *ptr = copy;

  for (i = 0; copy[i]; i++) {
    *ptr++ = path[i];
    if ('/' == path[i]) {
      i++;
      while ('/' == path[i]) i++;
      i--;
    }
  }

  *ptr = '\0';

  return copy;
}