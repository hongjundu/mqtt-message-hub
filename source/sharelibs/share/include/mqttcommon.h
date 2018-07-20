
#pragma once

/*
 MQTT QOS
*/

#ifndef QOS0
    #define QOS0 0
#endif

#ifndef QOS1
    #define QOS1 1
#endif

#ifndef QOS2
    #define QOS2 2
#endif

/*
 Topics

 CAT_TOPIC(MAIN_TOPIC, HEART_BEAT_SUB_TOPIC) 
*/

#define MAIN_TOPIC                    "hub"

#define ONLINE_SUB_TOPIC              "online"
#define MONITOR_STARTS_SUB_TOPIC      "start"
#define OFFLINE_SUB_TOPIC             "offline"
#define MSG_COUNT_SUB_TOPIC           "msgcount"

/*
mma/cmd/ALL
mma/cmd/{SN}
*/
#define CMD_SUB_TOPIC             "cmd"
#define CMD_ALL_SUB_TOPIC         "ALL"

#define MQTT_TOPIC_SPLITTER           "/"

#define CAT_TOPIC(main,sub)           main"/"sub

/*
 JSON node key
*/
/* mqttproxy http post data */
#define JSON_NODE_FROM_MODULE_KEY            "from"
#define JSON_NODE_MQTT_TOPIC_KEY             "topic"
#define JSON_NODE_MQTT_PAYLOAD_KEY           "payload"
#define JSON_NODE_MQTT_QOS_KEY               "qos"
#define JSON_NODE_MQTT_CACHE_PRIORITY        "cache_priority"

/* mma mqtt common */ 
#define JSON_NODE_SN_KEY                     "sn"
#define JSON_NODE_MAC_KEY                    "mac"
#define JSON_NODE_TIME_KEY                   "time"

/* mma/msgcount */
#define JSON_NODE_SENT_KEY                   "sent"
#define JSON_NODE_ONLINE_DISCARD_KEY         "onlinediscard"
#define JSON_NODE_OFFLINE_DISCARD_KEY        "offlinediscard"
#define JSON_NODE_OFFLINE_SENT_KEY           "offlinesent"
#define JSON_NODE_OFFLINE_QUEUE_CAPACITY     "offlinecap"

// /* return data such as command status etc. */
#define JSON_NODE_STATUS_KEY                 "status"  
#define JSON_NODE_ERROR_CODE_KEY             "code"
#define JSON_NODE_ERROR_KEY                  "err"
#define JSON_NODE_MESSAGE_KEY                "msg"

/*
JSON value
*/
#define VALUE_CACHE_PRIORITY_HIGH                 31
#define VALUE_CACHE_PRIORITY_NORMAL               16
#define VALUE_CACHE_PRIORITY_LOW                   8
#define VALUE_CACHE_PRIORITY_DEFAULT               8

/* cmd status */
#define VALUE_OK                             "ok"
#define VALUE_ERROR                          "err"
