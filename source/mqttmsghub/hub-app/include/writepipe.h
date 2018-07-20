/*
 *  File: writepipe.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#pragma once


/*
 open FIFO
*/
int open_write_pipe();

/*
 write buffer to named pipe, open it if the pipe if not open
*/
void write_pipe_buffer(char *buffer);

/*
 close write pipe when app exit
*/
int close_write_pipe();