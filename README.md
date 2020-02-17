# proxy\_app

The host proxy application for SEOS runnuing in QEMU.
This proxy implements the communication channels to the host machine in order to. The inner working mechanism to mux-demux channels is based on ChanMux/HDLC (https://wiki.hensoldt-cyber.systems/display/HEN/SEOS+ChanMUX%2C+UART+Proxy+and+Host-Bridge)

* simulate Non Volatile Memory
* access TAP interfaces
* test ChanMux
* route MQTT packets (HAR demo)


## Getting Started

The project builds a Linux command line utility

### Dependencies

* uart\_socket
* seos\_libs
* TinyFrame

### Build steps

    # create workspace folder
    mkdir proxy_app
    cd proxy_app

    # check out into folder "src"
    git clone --recursive ssh://git@bitbucket.hensoldt-cyber.systems:7999/hc/proxy_app.git src

    # run build, will create a folder "build" with the application binary
    src/build.sh

    # run application and connect to QEMU on TCP port 4444, listen on local
    # port 7999 for MQTT packets, open cloud connection to 51.144.118.31:8883
    build/proxy_app 4444 7999 51.144.118.31 8883
