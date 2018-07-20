/*
 *  File: msgcounter.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2018/02.
 *  Copyright 2018 ihangmei.com All rights reserved.
 */

#pragma once

#include "common.h"

void increase_sent_msg_count();

void increase_online_discarded_msg_count();

void increase_offline_discarded_msg_count();

void increase_offline_sent_msg_count();

void save_msg_count_to_file();

int get_total_msg_count(int *sentCount, int *onlineDiscardedCount, int *offlineDiscardedCount,int *offlineSentCount);

void clear_msg_count_file();
