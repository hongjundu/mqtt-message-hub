#pragma once

#ifndef BOOL
    typedef int BOOL;
#endif

#ifndef TRUE
    #define TRUE  1
#endif

#ifndef FALSE   
    #define FALSE 0
#endif

#ifndef NULL
    #define NULL 0
#endif

#ifndef SUCCESS
    #define SUCCESS 0
#endif

#ifndef FAILURE
    #define FAILURE -1
#endif

typedef unsigned char BYTE;

#define HTTP_REQUEST_TIMEOUT 10 /* secondes */

#define ERRNO_ERRORSTR errno,strerror(errno)

#define MICRO_SECONDS_PER_SECOND (1000 * 1000)
#define NANOSECONDS_PER_SECOND (1000 * 1000 * 1000)
    
#define APP_DATA_PATH        "mqttmsghub-data"
#define MQTT_OFFLINE_MSG_DIR "mqtt-offline-msgs"
#define FIFO_FILE_NAME       "mqtt_received_fifo"