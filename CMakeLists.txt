#
# Proxy Application
#
# Copyright (C) 2019-2020, Hensoldt Cyber GmbH
#

cmake_minimum_required(VERSION 3.10)

#-------------------------------------------------------------------------------
# configure and use the SDK

set(NO_KERNEL_BUILD ON CACHE BOOL "")

set(OS_SDK_PASSIVE_CMAKE ON)
include(${OS_SDK_SOURCE_PATH}/CMakeLists.txt)

os_set_config_file(system_config.h)

os_use_libs(
    OS_LIBS
    THIRD_PARTY_TINYFRAME
)

add_subdirectory(${OS_LIBS_DIR} sdk_build)


#-------------------------------------------------------------------------------
project(proxy_app CXX)

add_executable(${PROJECT_NAME}
    src/uart_socket/uart_hdlc.c
    src/uart_socket/uart_io_host.c
    src/proxy_simple.cpp
    src/ChannelAdmin.cpp
)

# compile all files with the C++ compiler
get_target_property(SOURCE_FILES ${PROJECT_NAME} SOURCES)
set_source_files_properties(${SOURCE_FILES} PROPERTIES LANGUAGE CXX)

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
        -Wno-delete-non-virtual-dtor
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        PAGE_SIZE=4096
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        pthread
        os_core_api
        os_libs
        third_party_tinyframe_static_cpp
)
