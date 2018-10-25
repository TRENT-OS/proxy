# Configs header files - Project dependent

project(configs C)

file(GLOB SOURCES "src/*.c" )

add_library(
    ${PROJECT_NAME} STATIC
    ${SOURCES}
)

target_include_directories(${PROJECT_NAME} PUBLIC include)

