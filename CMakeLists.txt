cmake_minimum_required(VERSION 3.10)

#-------------------------------------------------------------------------------
# configure and use the SDK

set(PAGE_SIZE_VALUE 4096 CACHE STRING "" FORCE)
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
    uart_socket/src/uart_hdlc.c
    uart_socket/src/uart_io_host.c
    src/proxy_simple.cpp
    src/ChannelAdmin.cpp
)

# compile all files with the C++ compiler
get_target_property(SOURCE_FILES ${PROJECT_NAME} SOURCES)
set_source_files_properties(${SOURCE_FILES} PROPERTIES LANGUAGE CXX)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        uart_socket/src
        src
)

target_compile_options(${PROJECT_NAME}
    PRIVATE
        -Wall
        -Werror
        -pthread
        -Wno-delete-non-virtual-dtor
)

target_link_libraries(${PROJECT_NAME}
    pthread
    seos_core_api
    seos_libs
    third_party_tinyframe_static_cpp
)
