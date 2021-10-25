# Proxy App

The host proxy application for TRENTOS running in QEMU.
This proxy implements the communication channels to the host machine in order to

* simulate Non Volatile Memory
* access TAP interfaces
* test ChanMux

The inner working mechanism to mux-demux channels is based on ChanMux/HDLC (https://wiki.hensoldt-cyber.systems/display/HEN/SEOS+ChanMUX%2C+Linux+Proxy+App).

## Getting Started

The project builds a Linux command line utility

### Dependencies

* uart\_socket
* seos\_sandbox
* TinyFrame

### Build steps

create workspace folder

    mkdir proxy_app
    cd proxy_app

check out into folder "src"

    git clone --recursive ssh://git@bitbucket.hensoldt-cyber.systems:7999/hc/mqtt_proxy_demo.git src

run build, will create a folder "build" with the application binary

    src/build.sh <path-to-OS-SDK>

### Synopsis
    ./proxy_app -c [<connectionType>:<Param>] -t [tap_mode]


### Example usage
Run application and connect to QEMU on TCP port 4444, allowing for the creation and usage of TAP devices

    build/proxy_app -c TCP:4444 -t 1
