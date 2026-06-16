cmake_minimum_required(VERSION 3.15)
project(ludicrous_engine)

find_package(JNI REQUIRED)

add_library(ludicrous_engine SHARED
    ludicrous_engine.c
)

target_include_directories(ludicrous_engine PRIVATE
    ${JNI_INCLUDE_DIRS}
)

target_compile_options(ludicrous_engine PRIVATE -O3 -march=native)

set_target_properties(ludicrous_engine PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/output"
)