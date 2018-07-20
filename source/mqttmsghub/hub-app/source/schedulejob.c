/*
 *  File: schedulejob.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#include "schedulejob.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <memory.h>
#include <time.h>
#include <signal.h>
#include "common.h"
#include "logger.h"
#include "mqttclientwrapper.h"
#include "msgcounter.h"

#define BASE_SCHEDULE_INTERVAL 10  /* in seconds */
static int g_totalSeconds = 0;

static int check_mqtt_connection(int);
static int sync_msg_count_file(int);

static schedule_job schedule_job_list[] = 
{
    { (char*)"chkconninterval",                  30,        check_mqtt_connection     },
    { (char*)"syncmsgcountinterval",             5,         sync_msg_count_file       },
    { NULL,                                      0,         NULL                      }   /* END tag */
};

static void sig_alarm_handler(int signo)
{
    g_totalSeconds += BASE_SCHEDULE_INTERVAL;

    Log_debug("sig_alarm_handler %d", g_totalSeconds);

    Log_info("Total seconds escaped: %d", g_totalSeconds);

    schedule_job *job = &schedule_job_list[0];
    while (job->name) {
        if (job->interval > 0 && (g_totalSeconds % job->interval) == 0) {
            job->doJob(g_totalSeconds);
        }
        job++;
    }

    if (g_totalSeconds >= INT_MAX - BASE_SCHEDULE_INTERVAL) {   /* avoid overflow */
        g_totalSeconds = 0;
    }
}

/*
 setup timer
*/
int setup_schedule_timer()
{
    Log_debug("setup_schedule_timer");

    return timer_helper_setup_timer(BASE_SCHEDULE_INTERVAL, sig_alarm_handler);
}

/*
 free timer
*/
int destroy_schedule_timer()
{
    Log_debug("destroy_schedule_timer");

     return timer_helper_destroy_timer();
}

static int check_mqtt_connection(int totalSeconds)
{
    Log_debug("check_mqtt_connection %d", totalSeconds);

    if (MQTTClientWrapper_isConnected()) {
        Log_info("<<<<<<< mqtt is CONNECTED >>>>>>")

        if (MQTTClientWrapper_isSubscribed()) {
            Log_info("topics is subscribled");
        }
        else {
            Log_warn("topic is not subscribled, subscribe...");
            MQTTClientWrapper_subscribe();
        }
    }
    else {
    #ifdef USE_BUILD_IN_MQTT_RECONNECT
        Log_info("<<<<<<< MQTT client is NOT CONNECTED: Use build-in auto reconnect >>>>>>");

        if (!MQTTClientWrapper_hasEverConnected()) {
            Log_info("MQTT has not ever connected, Reconnecting....");
            MQTTClientWrapper_connect();
        }

    #else
        Log_warn("<<<<<<< MQTT client is NOT CONNECTED >>>>>> Manual reconnecting...");
        MQTTClientWrapper_connect();
    #endif
    }

    return 0;
}

static int sync_msg_count_file(int totalSeconds)
{
    Log_debug("sync_msg_count_file %d", totalSeconds);
    save_msg_count_to_file();
    return 0;
}
