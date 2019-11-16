cmake_minimum_required(VERSION 3.10)

#-------------------------------------------------------------------------------
# configure and use sandbox

set(PAGE_SIZE_VALUE 4096 CACHE STRING "" FORCE)
set(NO_KERNEL_BUILD ON CACHE BOOL "")

set(SEOS_SANDBOX_PASSIVE_CMAKE ON)
include(${SANDBOX_SOURCE_PATH}/CMakeLists.txt)

seos_set_config_file(configs/include/Debug_Config.h)

seos_use_libs(
    SEOS_LIBS
)

add_subdirectory(${SANDBOX_SOURCE_PATH}/projects/libs sandbox_build)


#-------------------------------------------------------------------------------
# PicoTCP

set(PICO_PATH "picotcp/build/lib")
set(PICO_INCLUDE_PATH "picotcp/build/include")
set(PICO_BSD_PATH "picotcp-bsd")


#-------------------------------------------------------------------------------

project(mqtt_proxy CXX)

add_executable(${PROJECT_NAME}
    uart_socket/src/uart_hdlc.c
    uart_socket/src/uart_io_host.c
    TinyFrame/demo/utils.c
    TinyFrame/TinyFrame.c
    src/proxy_simple.cpp
    src/SocketAdmin.cpp
)

# compile all files with the C++ compiler
get_target_property(SOURCE_FILES ${PROJECT_NAME} SOURCES)
set_source_files_properties(${SOURCE_FILES} PROPERTIES LANGUAGE CXX)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        uart_socket/src
        TinyFrame/demo
        src
    PUBLIC
        ${PICO_INCLUDE_PATH}
        ${PICO_BSD_PATH}
        ${PICO_PATH}
)

target_compile_options(${PROJECT_NAME}
    PRIVATE
        -pthread
)

target_link_libraries(${PROJECT_NAME}
    seos_libs
    pthread
    ${CMAKE_CURRENT_SOURCE_DIR}/${PICO_BSD_PATH}/build/libpico-bsd.a
    dl
    ${CMAKE_CURRENT_SOURCE_DIR}/${PICO_PATH}/libpicotcp.a
)

#-------------------------------------------------------------------------------

add_executable(SendMqttPublish
    mqtt_client/src/SendMqttPublish.cpp
)

target_include_directories(SendMqttPublish
    PRIVATE
        src
)


#-------------------------------------------------------------------------------

add_executable(MqttServer
    mqtt_server/src/MqttServer.cpp
)

target_include_directories(MqttServer
    PRIVATE
        src
        uart_socket/src
        TinyFrame/demo
)

target_compile_options(MqttServer
    PRIVATE
        -pthread
)

target_link_libraries(MqttServer
    pthread
)
