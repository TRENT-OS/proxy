cmake_minimum_required(VERSION 3.10)

project(mqtt_proxy C CXX)

file(GLOB SOURCES "src/*.c" "src/*.cpp"
    "uart_socket/src/uart_hdlc.c"
    "uart_socket/src/uart_io_host.c"
    "TinyFrame/demo/utils.c"
    "TinyFrame/TinyFrame.c")

set_source_files_properties( ${SOURCES} PROPERTIES LANGUAGE CXX )

add_executable(${PROJECT_NAME} ${SOURCES})

include_directories("seos_libs/src")
include_directories("TinyFrame/demo")

target_compile_options(${PROJECT_NAME} PRIVATE -pthread)
target_include_directories(${PROJECT_NAME} PRIVATE uart_socket/src)
target_include_directories(${PROJECT_NAME} PUBLIC seos_libs/include)
target_include_directories(${PROJECT_NAME} PUBLIC configs/include)
target_link_libraries(${PROJECT_NAME} pthread)
