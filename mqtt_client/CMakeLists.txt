cmake_minimum_required(VERSION 3.10)

project(mqtt_client CXX)

add_executable(${PROJECT_NAME}
    src/SendMqttPublish.cpp
)

# compile all files with the C++ compiler
get_target_property(SOURCE_FILES ${PROJECT_NAME} SOURCES)
set_source_files_properties(${SOURCE_FILES} PROPERTIES LANGUAGE CXX)


target_include_directories(${PROJECT_NAME}
    PRIVATE
        uart_socket/src
)


target_compile_options(${PROJECT_NAME} PRIVATE -pthread)
target_link_libraries(${PROJECT_NAME} pthread)
