#
# Proxy Application
#
# Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
# 
# SPDX-License-Identifier: GPL-2.0-or-later
#
# For commercial licensing, contact: info.cyber@hensoldt.net
#

cmake_minimum_required(VERSION 3.10)

#-------------------------------------------------------------------------------
# use the SDK
find_package("os-sdk" REQUIRED)
os_sdk_create_config_project("os_sdk_config" "system_config.h")
os_sdk_import_core_api()
os_sdk_import_libs()


#-------------------------------------------------------------------------------
project(proxy_app C CXX)

add_executable(${PROJECT_NAME}
    src/uart_socket/uart_hdlc.c
    src/uart_socket/uart_io_host.c
    src/proxy_simple.cpp
    src/ChannelAdmin.cpp
)

# compile all files with the C++ compiler
get_target_property(SOURCE_FILES ${PROJECT_NAME} SOURCES)
set_source_files_properties(${SOURCE_FILES}
    PROPERTIES
        LANGUAGE CXX
        COMPILE_OPTIONS "-Wno-delete-non-virtual-dtor"
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        src/uart_socket
        src
)

target_compile_options(${PROJECT_NAME}
    PRIVATE
        -Wall
        -Werror
        -pthread
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        os_sdk_config
        pthread
        os_core_api
        lib_compiler
        lib_debug
        lib_utils
        third_party_tinyframe_static_cpp
)
