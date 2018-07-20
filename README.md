# mqtt-message-hub

POSIX-C mqtt message hub 

* for demonstration purpose
* tested on ubuntu 64 bit
* doesn't work on OSX

## Before Building

* Config mqtt endpoint ```mqttserver``` in ```source/mqttmsghub/conf/mqttmsghub.ini```
* Config ```MQTT_USER``` and ```MQTT_PASSWD``` in ```source/sharelibs/share/include/secdef.h```

## Build 

- Local Build

    cd ./source
    make

- Cross Build

    cd ./source
    make CROSS=arm-linux-gnueabihf- TARGET_FOLDER=arm

## Run
    
    cd ./build/local/bin
    ./mqttmsghub

## Send MQTT Message via ```mqttmsghub``` HTTP API

    data='{"topic":"test","payload":{"Hello":"World","Content":"This is a test message"}}'
    url='http://127.0.0.1:8066/mqtt/send'
    curl -d "$data" "$url"