/*
 *  File: offlinemsgcache.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#pragma once

#include <sys/types.h>

/*
  save offline message (in memroy & file)
*/
int enqueue_offline_message(char *message, int cache_priority, char *errorMsg, int64_t timestamp);

/*
 set the cache capacity, default is 1000
 */
void set_offline_cache_capacity(int capacity);

/*
 returns cache capacity
*/
int get_offline_cache_capacity();

/*
 get pending offline messages count
*/
int offline_messages_count();

/*
 load cache from disk
*/
int load_offline_messages();

/*
 send pending messages async
*/
int asyn_send_offline_messages();