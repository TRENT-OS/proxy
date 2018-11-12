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

    # check out
    git clone --recursive -b master ssh://git@bitbucket.hensoldt-cyber.systems:7999/hc/mqtt_proxy_demo.git

    # build
    mkdir build
    cd build
    cmake .. -G Ninja
    ninja

    # run
    # The command line parameter "pseudoterminal" is the name of the device QEMU has mapped the serial port to. Usually it is something like /dev/pts/4
    ./mqtt_proxy_demo pseudoterminal

