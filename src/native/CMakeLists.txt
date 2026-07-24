cmake_minimum_required(VERSION 3.15)
project(ludicrous_engine)

find_package(JNI REQUIRED)
find_package(Vulkan REQUIRED)

add_library(ludicrous_engine SHARED src/ludicrous_engine.c)

add_library(stack_builder STATIC
    src/stack_builder.c
    src/noise_gen.c
    src/util.c
)

add_executable(test_cases src/test_cases.c)

set(le_sources
    logger.c
    noise_gen.c
    vulkan_util.c
    shader.c
    yyjson.c
)

list(TRANSFORM le_sources PREPEND "src/")

target_sources(ludicrous_engine PRIVATE ${le_sources})

target_link_libraries(ludicrous_engine PRIVATE Vulkan::Vulkan stack_builder)
target_link_libraries(test_cases PRIVATE stack_builder m)

target_include_directories(ludicrous_engine PRIVATE
    ${JNI_INCLUDE_DIRS}
    include
)
target_include_directories(stack_builder PRIVATE include)
target_include_directories(test_cases PRIVATE include)

target_compile_options(ludicrous_engine PRIVATE -O1 -march=native)

set_target_properties(ludicrous_engine PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/output"
)
set_target_properties(stack_builder PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)