/*
 *  File: sysutils.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#pragma once

#include <stdio.h>
/* 
 app level utility functions
*/

/*
 create a daemon process
*/
void daemonize();

/*
 Ensure the app is running only one instance
*/
int single_instance_app(const char *lockFileName);

/*
 execute a system command
*/
int cmd_system(const char *command, char *buf, int bufSize);


/*
 Kill a running process
*/
int kill_process(const char *name);
