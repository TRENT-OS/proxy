# mqtt_proxy_demo

The host proxy application for the High Assurance Router (HAR) running in seL4 in QEMU.
This proxy implements the following functionality:

* it opens a server side socket and forwards all incoming traffic from accepted connections to the HAR using the uart socket logical channel id 0x00.
* it opens a socket to the Azure Cloud and forwards all incoming traffic to the HAR using the uart socket logical channel id 0x01.
* it listens to all traffic arriving from the HAR on uart socket logical channel id 0x01 and forwards it to the Azure Cloud.

## Getting Started

The project builds a Linux command line utility

### Dependencies

* uart\_socket
* seos\_libs
* TinyFrame

### Build steps

    # create workspace folder
    mkdir mqtt_proxy_demo
    cd mqtt_proxy_demo

    # check out into folder "src"
    git clone --recursive ssh://git@bitbucket.hensoldt-cyber.systems:7999/hc/mqtt_proxy_demo.git src

    # run build, will create a folder "build" with the application binary
    src/build.sh

    # run application and connect to QEMU on TCP port 4444, listen on local
    # port 7999 for MQTT packets, open cloud connection to 51.144.118.31:8883
    build/mqtt_proxy 4444 7999 51.144.118.31 8883
