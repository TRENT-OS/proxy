project(mqtt_proxy C CXX)

cmake_minimum_required(VERSION 3.10)

file(GLOB SOURCES "src/*.c" "src/*.cpp"
    "uart_socket/src/uart_hdlc.c"
    "uart_socket/src/uart_io_host.c"
    "src/TinyFrame.c")

set_source_files_properties( ${SOURCES} PROPERTIES LANGUAGE CXX )

add_executable(${PROJECT_NAME} ${SOURCES})

include_directories("seos_libs/src")

add_subdirectory(configs)

target_compile_options(${PROJECT_NAME} PRIVATE -pthread)
target_include_directories(${PROJECT_NAME} PRIVATE uart_socket/src)
target_link_libraries(${PROJECT_NAME} configs pthread)
