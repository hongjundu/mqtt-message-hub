/*
 *  File: sysutils.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#include "sysutils.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include "filepathutils.h"
#include "processstatus.h"

#ifdef _WIN32
#define PATH_SEPARATOR   '\\'
#else
#define PATH_SEPARATOR   '/'
#endif

/*************************************************************************************
 Publics
**************************************************************************************/
/*
 create a daemon process
*/
void daemonize()
{
    /* Our process ID and Session ID */
    pid_t pid, sid;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* Open any logs here */

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
         /* Log the failure */
        exit(EXIT_FAILURE);
    }


    pid = fork();

    if (pid < 0)   {
        exit(EXIT_FAILURE);
    }

    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    /* Change the current working directory */
    if ((chdir("/")) < 0) {
            /* Log the failure */
       exit(EXIT_FAILURE);
    }

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /* Redirect to empty */
    int fd = open("/dev/null", O_RDWR);
    dup2(fd,STDIN_FILENO);
    dup2(fd,STDOUT_FILENO);
    dup2(fd,STDERR_FILENO);
}

/*
 Ensure the app is running only one instance
*/
int single_instance_app(const char *lockFileName)
{
    char lockFilePath[_POSIX_PATH_MAX] = { 0 };
    local_path(lockFilePath);
    strcat(lockFilePath, "/");
    strcat(lockFilePath, lockFileName);

    int fd = open(lockFilePath, O_RDWR|O_CREAT, 0666);
    if(fd < 0)
    {
        printf("Open file %s failed.\n", lockFilePath);
        return -1;
    }

    int locked = lockf(fd, F_TEST, 0); //返回0表示未加锁或者被当前进程加锁；返回-1表示被其他进程加锁
    if(locked < 0)
    {
        printf("The program has been launched, exit now!\n");
        exit(EXIT_SUCCESS);
    }

    locked = lockf(fd, F_LOCK, 0);  //参数使用F_LOCK，则如果已经加锁，则阻塞到前一个进程释放锁为止，参数0表示对整个文件加锁
    if(locked < 0)
    {
        printf("The program has been launched, exit now!\n");
        exit(EXIT_SUCCESS);
    }

    return 0;
}


/*
 execute a system command
*/
int cmd_system(const char *command, char *buf, int bufSize)
{
    printf("cmd_system : %s\n", command);

    char buf_ps[1024*10] = { 0 };
    int rc = -1;
    FILE *fpRead = 0;

    fpRead = popen(command, "r");

    if (!fpRead) {
        printf("popen NULL\n");
        return rc;
    }

    memset(buf_ps, 0, sizeof(buf_ps));
    memset(buf, 0, bufSize);

    while(fgets(buf_ps, sizeof(buf_ps), fpRead) != NULL) {
        if(strlen(buf) + strlen(buf_ps) > bufSize - 1) {
            break;
        }
        strcat(buf, buf_ps);
    }

    if(fpRead != NULL) {
        rc = pclose(fpRead);
    }

    int len = strlen(buf);

    if(len > 0 && '\n' == buf[len - 1]) {
        buf[len - 1] = 0;
    }

    if (rc < 0) {
        return rc;
    }

    return WEXITSTATUS(rc);
}

/*
 Kill a running process
*/
int kill_process(const char *name)
{
    printf("kill_process: %s\n", name);

    pid_t pid = get_process_id(name);
    if (pid > 0) {
        char cmd[32] = { 0 };
        sprintf(cmd, "kill %d", pid);

        int code = system(cmd);
        int exitCode = WIFEXITED(code);
        int exitStatus = WEXITSTATUS(code);

        printf("kill process '%s' exitCode: %d exitStatus: %d\n", name, exitCode, exitStatus);

        if (!exitCode || exitStatus != 0) {
            printf("kill process '%s' failed, exitCode: %d exitStatus: %d\n", name, exitCode, exitStatus);
            return -1;
        }
        else {
            printf("kill process '%s' successfully\n", name);
            return 0;
        }
    }
    else {
        printf("process: %s not found\n", name);
        return -1;
    }
}
