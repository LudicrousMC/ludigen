cmake_minimum_required(VERSION 3.15)
project(ludicrous_engine)

find_package(JNI REQUIRED)
find_package(Vulkan REQUIRED)

add_library(ludicrous_engine SHARED
    src/ludicrous_engine.c
)

target_link_libraries(ludicrous_engine PRIVATE Vulkan::Vulkan)

target_include_directories(ludicrous_engine PRIVATE
    ${JNI_INCLUDE_DIRS}
    "include"
)

target_compile_options(ludicrous_engine PRIVATE -O3 -march=native)

set_target_properties(ludicrous_engine PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/output"
)