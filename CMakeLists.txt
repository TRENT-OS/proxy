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

add_subdirectory(${SEOS_LIBS_DIR} sandbox_build)

project(proxy_app CXX)

add_executable(${PROJECT_NAME}
    uart_socket/src/uart_hdlc.c
    uart_socket/src/uart_io_host.c
    TinyFrame/demo/utils.c
    TinyFrame/TinyFrame.c
    src/proxy_simple.cpp
    src/ChannelAdmin.cpp
)

# compile all files with the C++ compiler
get_target_property(SOURCE_FILES ${PROJECT_NAME} SOURCES)
set_source_files_properties(${SOURCE_FILES} PROPERTIES LANGUAGE CXX)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        uart_socket/src
        TinyFrame/demo
        src
)

target_compile_options(${PROJECT_NAME}
    PRIVATE
        -pthread
        -Wno-delete-non-virtual-dtor
)

target_link_libraries(${PROJECT_NAME}
    seos_core_api
    seos_libs
    pthread
)
