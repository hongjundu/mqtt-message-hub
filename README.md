# mqtt-message-hub

POSIX-C mqtt message hub 

## Notice

This project is only for demonstration purpose, verified on ubuntu64

## Usage

### Pre-Build

* Config mqtt endpoint ```mqttserver``` in ```source/mqttmsghub/conf/mqttmsghub.ini```
* Config ```MQTT_USER``` and ```MQTT_PASSWD``` in ```source/sharelibs/share/include/secdef.h```

### Build 

    cd ./source
    make

### Run
    
    ./build/local/bin/mqttmsghub

### Send MQTT Message via ```mqttmsghub``` HTTP API

    data='{"topic":"test","payload":{"Hello":"World","Content":"This is a test message"}}'
    url='http://127.0.0.1:8066/mqtt/send'
    curl -d "$data" "$url"