find_program(GLSLC_EXECUTABLE glslc)
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "Could not find glslc compiler. Ensure the Vulkan SDK is installed.")
endif()

find_program(XXD_EXECUTABLE xxd)
if(NOT XXD_EXECUTABLE)
    message(FATAL_ERROR "Could not find xxd. Ensure xxd is installed.")
endif()

set(SHADER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/compute/eval.comp")
set(SPV_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/eval.comp.spv")
set(EVAL_SHADER "${CMAKE_CURRENT_BINARY_DIR}/eval.comp.h")

add_custom_command(
    OUTPUT ${SPV_OUTPUT}
    COMMAND ${GLSLC_EXECUTABLE} ${SHADER_SRC} -o ${SPV_OUTPUT} -I "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DEPENDS ${SHADER_SRC}
    COMMENT "Compiling Compute Shader: ${SHADER_SRC}"
)

add_custom_command(
    OUTPUT ${EVAL_SHADER}
    COMMAND ${XXD_EXECUTABLE} -i -n eval_spv ${SPV_OUTPUT} > ${EVAL_SHADER}
    DEPENDS ${SPV_OUTPUT}
    COMMENT "Generating Shader Header File: ${EVAL_SHADER}"
)

#add_custom_target(eval_shader DEPENDS ${EVAL_SHADER})
