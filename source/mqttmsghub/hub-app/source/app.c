/*
 *  File: app.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <limits.h> 
#include "sysutils.h"
#include "filepathutils.h"
#include "stringutils.h"
#include "logger.h"
#include "common.h"
#include "mqttclientwrapper.h"
#include "threadpoolwrapper.h"
#include "schedulejob.h"
#include "msgrevhandler.h"
#include "devicesn.h"
#include "httpserver.h"
#include "writepipe.h"
#include "offlinemsgcache.h"
#include "filepathutils.h"
#include "msgcounter.h"
#include "config.h"
#include "version.h"
#include <execinfo.h>
#include "netutils.h"

#ifdef PAYLOAD_ENCRYPT
#include "tomcrypt.h"
#endif

static void usage()
{
    printf("Mqtt Message Hub\n");
    printf("Usage: mqttmsghub, where options are:\n");
    printf("  --help This message\n");
    printf("  --version App version string\n");
    printf("  --nodaemon Run the app as a none deamon app\n");

    exit(EXIT_SUCCESS);
}

static void version_string(char *buffer) 
{
    sprintf(buffer, "%s %s", APP_VERSION, BUILD_TIMESTAMP);
}

static void version()
{
    char version[64] = { 0 };
    version_string(version);
    printf("%s\n", version);
    exit(EXIT_SUCCESS);
}


static void run_test() 
{
    printf("TODO: run automation testing...\n");
    exit(EXIT_SUCCESS);
}

struct Args {
    BOOL isDaemon;
    BOOL isTest;
} args = { TRUE, FALSE };

static void parse_args(int argc, char **argv) 
{
    int count = 1;

    while (count < argc) {
        if (string_equals(argv[count], "--help", TRUE)) {
            usage();
        }
        if (string_equals(argv[count], "--version", TRUE)) {
            version();
        }
        else if (string_equals(argv[count], "--test", TRUE)) {
            run_test();
        }
        else if (string_equals(argv[count], "--nodaemon", TRUE)) {
            args.isDaemon = 0;
        }
        count++;
    }

}

static void write_run_version()
{
    char version[64] = { 0 };
    version_string(version);
    write_app_run_version_file(MQTTMSGHUB_APP_NAME,version);
}

static void clear_run_version() 
{
    delete_app_run_version_file(MQTTMSGHUB_APP_NAME);
}

static void segv_handler(int sig)
{
    Log_error("segv_handler %d", sig);

    void *array[10];
    size_t size = 0;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);

    exit(EXIT_FAILURE);
}

static void on_mqtt_client_connected() 
{
    Log_debug("on_mqtt_client_connected");

    char onlineMessage[64] = { 0 };
    time_t timestamp = time(NULL);
    sprintf(onlineMessage, "{\"%s\":\"%s\",\"%s\":%lld}", JSON_NODE_SN_KEY, get_device_sn(), JSON_NODE_TIME_KEY, (long long int)timestamp);

    /*
     Report mqtt online
    */
    Log_info("send online message: %s", onlineMessage);
    MQTTClientWrapper_sendMessage(CAT_TOPIC(MAIN_TOPIC, ONLINE_SUB_TOPIC), onlineMessage, MAXQOS);

    if (offline_messages_count() > 0) {
        asyn_send_offline_messages();
    }

    int sentCount = 0, onlineDiscardedCount = 0, offlineDiscardedCount = 0, offlineSentCount = 0;
    get_total_msg_count(&sentCount, &onlineDiscardedCount, &offlineDiscardedCount, &offlineSentCount);
    if (sentCount > 0 || onlineDiscardedCount > 0 || offlineDiscardedCount > 0 || offlineSentCount > 0) {
        char msgCountMessage[256] = { 0 };
        sprintf(msgCountMessage, "{\"%s\":%d,\"%s\":%d,\"%s\":%d,\"%s\":%d,\"%s\":%d,\"%s\":\"%s\",\"%s\":%lld}", 
            JSON_NODE_SENT_KEY, sentCount,
            JSON_NODE_ONLINE_DISCARD_KEY, onlineDiscardedCount,
            JSON_NODE_OFFLINE_DISCARD_KEY, offlineDiscardedCount,
            JSON_NODE_OFFLINE_SENT_KEY, offlineSentCount,
            JSON_NODE_OFFLINE_QUEUE_CAPACITY, get_offline_cache_capacity(),
            JSON_NODE_SN_KEY, get_device_sn(), 
            JSON_NODE_TIME_KEY, (long long int)timestamp);
        Log_info("send msg count message: %s", msgCountMessage);
        if (MQTTClientWrapper_sendMessage(CAT_TOPIC(MAIN_TOPIC, MSG_COUNT_SUB_TOPIC), msgCountMessage, MAXQOS) == SUCCESS) {
            clear_msg_count_file();
        }
    }
}

int run_app(int argc, char **argv) {
    int exitCode = EXIT_SUCCESS;
    int rc = -1;

    #ifdef PAYLOAD_ENCRYPT
    register_cipher(&aes_desc);
    #endif

    parse_args(argc,argv);

    if (args.isDaemon) {
        printf("Run app in daemon mode\n");
        daemonize();
    }
    else {
        printf("Run app in nodaemon mode\n");
    }

    single_instance_app(MQTTMSGHUB_LOCK_FILE);
    write_run_version();

    Log_init(LOG_CATETORY);

    Log_info("app starts, version: %s %s", APP_VERSION, BUILD_TIMESTAMP);

    /*
     Crash hanlder
    */
    signal(SIGSEGV, segv_handler);

    /*
     Ensure device sn 
    */
    while (string_is_empty(get_device_sn())) {
        Log_warn("device SN is empty, wait 1 second and retry...");
        usleep(1 * MICRO_SECONDS_PER_SECOND);
    }

    Log_info("devcie SN: %s", get_device_sn());

    set_offline_cache_capacity(get_config_offline_cache_capacity());

    /*
     Make sure FIFO file is created
    */
    open_write_pipe();

    /*
     Load offline mqtt message from cache
    */
    Log_info("Loading offline messages...");
    load_offline_messages();

    Log_info("Init MQTT client...");
    rc = MQTTClientWrapper_init();
    MQTTClientWrapper_setMessageReceivedCallback(msg_received_handler);
    MQTTClientWrapper_setConnectedCallback(on_mqtt_client_connected);
    if (rc == SUCCESS) {
        MQTTClientWrapper_connect();
    }
    else {
        Log_error("Init MQTT client failed, error: %d", rc);
        goto EXIT;
    }

    if (setup_schedule_timer() < 0) {
        goto EXIT;
    }

    rc = HttpServer_run();
    if (rc < 0) {
        Log_fatal("HttpServer_run failed, error : %d", rc);
        goto EXIT;
    }

    EXIT:
    MQTTClientWrapper_deinit();
    destroy_schedule_timer();
    close_write_pipe();
    ThreadPoolWrapper_deinit(FALSE);

    Log_info("Exiting process with code %d ...", exitCode);
    Log_deinit();

    clear_run_version();

    return exitCode;
}