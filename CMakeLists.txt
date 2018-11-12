cmake_minimum_required(VERSION 3.10)

project(mqtt_proxy CXX)

add_executable(${PROJECT_NAME}
    uart_socket/src/uart_hdlc.c
    uart_socket/src/uart_io_host.c
    TinyFrame/demo/utils.c
    TinyFrame/TinyFrame.c
    src/proxy_simple.cpp
)

# compile all files with the C++ compiler
get_target_property(SOURCE_FILES ${PROJECT_NAME} SOURCES)
set_source_files_properties(${SOURCE_FILES} PROPERTIES LANGUAGE CXX)


target_include_directories(${PROJECT_NAME}
    PRIVATE
        configs/include
        uart_socket/src
        TinyFrame/demo
        seos_libs/include
)


target_compile_options(${PROJECT_NAME} PRIVATE -pthread)
target_link_libraries(${PROJECT_NAME} pthread)
