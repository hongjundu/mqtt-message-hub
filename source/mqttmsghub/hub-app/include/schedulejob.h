/*
 *  File: schedulejob.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#pragma once
#include "common.h"
#include "timerhelper.h"

/*
 setup timer
*/
int setup_schedule_timer();

/*
 free timer
*/
int destroy_schedule_timer();