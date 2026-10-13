cmake_minimum_required(VERSION 3.15)
project(ludicrous_engine)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Retrieve yyjson library
include(FetchContent)
FetchContent_Declare(
    yyjson
    GIT_REPOSITORY https://github.com/ibireme/yyjson.git
    GIT_TAG 0.12.0
)
FetchContent_MakeAvailable(yyjson)

# Retrieve tinycthread library
include(FetchContent)
FetchContent_Declare(
    tinycthread
    GIT_REPOSITORY https://github.com/tinycthread/tinycthread.git
    GIT_TAG v1.1
)
FetchContent_MakeAvailable(tinycthread)

add_library(ludicrous_engine SHARED src/ludicrous_engine.c)
include(JNI)
include(Vulkan)
include(GenEvalShader)

add_library(stack_builder STATIC
    src/stack_builder.c
    src/noise_gen.c
    src/util.c
)

add_executable(test_cases src/test_cases.c)

# Ludicrous Engine required source files
set(le_sources
    logger.c
    noise_gen.c
    vulkan_util.c
    shader.c
)
list(TRANSFORM le_sources PREPEND "src/")
list(APPEND le_sources "${tinycthread_SOURCE_DIR}/source/tinycthread.c")
target_sources(ludicrous_engine PRIVATE ${le_sources} ${EVAL_SHADER})

target_link_libraries(ludicrous_engine PRIVATE Vulkan::Vulkan JNI::JNI yyjson stack_builder)
target_link_libraries(test_cases PRIVATE stack_builder)
if(UNIX AND NOT APPLE)
    target_link_libraries(test_cases PRIVATE m)
endif()

target_include_directories(ludicrous_engine PRIVATE
    include
    ${JNI_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    "${tinycthread_SOURCE_DIR}/source"
)
target_include_directories(stack_builder PRIVATE include)
target_include_directories(test_cases PRIVATE include)

target_compile_options(ludicrous_engine PRIVATE -O3 -march=native)

set_target_properties(ludicrous_engine PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output"
)
set_target_properties(stack_builder PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)