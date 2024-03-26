cmake_minimum_required(VERSION 3.13)
project(walle)

# === DIRS ===
set(CMAKE_CURRENT_BINARY_DIR "${CMAKE_CURRENT_LIST_DIR}/build")
set(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src")
set(INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(BUILD_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/src)
set(BUILD_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
set(SCRIPTS_DIR "${CMAKE_CURRENT_LIST_DIR}/scripts")
set(GPERF_DIR "${CMAKE_CURRENT_LIST_DIR}/gperf")
# ============

# == CFLAGS ==
# modified from https://cmake.org/pipermail/cmake/2013-January/053117.html
set(FILENAME_MACRO "-D__FILENAME__='\"$(patsubst CMakeFiles/${PROJECT_NAME}.dir/%.o,%,$@)\"'")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FILENAME_MACRO}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DENABLE_DEBUG -Wall -Wextra -Wno-implicit-fallthrough -Wno-missing-field-initializers -ggdb")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
# ============

# === GL3W ===
set(GL3W_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/gl3w")
execute_process(COMMAND python gl3w_gen.py --root "${CMAKE_CURRENT_BINARY_DIR}" WORKING_DIRECTORY "${GL3W_SRC_DIR}")
# ============

# == SHADERS =
file(GLOB SHADER_SRC "${CMAKE_CURRENT_LIST_DIR}/shaders/*.glsl")
set(SHADER_HEADER "${BUILD_INCLUDE_DIR}/shaders.h")
execute_process(COMMAND "${SCRIPTS_DIR}/shader_compile.sh" "${SHADER_HEADER}" ${SHADER_SRC})
# ============

# == CONFIG ==
string(TOUPPER "${PROJECT_NAME}" PROJECT_NAME_CAPS)
configure_file("${CMAKE_CURRENT_LIST_DIR}/config.h.in" "${BUILD_INCLUDE_DIR}/config.h")
# ============

# == GPERF ==
set(GPERF_SRC_DIR "${BUILD_SRC_DIR}/gperf")
set(GPERF_INCLUDE_DIR "${BUILD_INCLUDE_DIR}/gperf")
file(GLOB GPERF_INPUT "${GPERF_DIR}/*.gperf")
foreach(gperf_input_file ${GPERF_INPUT})
	execute_process(COMMAND sh -c "echo ${gperf_input_file} | sed 's|${GPERF_DIR}/\\(.*\\)\\.gperf|${GPERF_SRC_DIR}/\\1.c|'" OUTPUT_VARIABLE gperf_output_source OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND sh -c "echo ${gperf_input_file} | sed 's|${GPERF_DIR}/\\(.*\\)\\.gperf|${GPERF_INCLUDE_DIR}/\\1.h|'" OUTPUT_VARIABLE gperf_output_header OUTPUT_STRIP_TRAILING_WHITESPACE)
	add_custom_command(
		COMMAND ${SCRIPTS_DIR}/gperf.sh --readonly-tables --strncmp-no-null "${gperf_input_file}" --struct-type --output-source "${gperf_output_source}" --output-header "${gperf_output_header}"
		DEPENDS "${gperf_input_file}"
		OUTPUT "${gperf_output_source}" "${gperf_output_header}"
	)
	list(APPEND GPERF_OUTPUT "${gperf_output_source}" "${gperf_output_header}")
endforeach()
add_library(gperf OBJECT ${GPERF_OUTPUT})
set_target_properties(gperf PROPERTIES LINKER_LANGUAGE C)
target_include_directories(gperf PUBLIC "${GPERF_INCLUDE_DIR}" "${INCLUDE_DIR}")
# ===========

# === MAIN ===
file(GLOB SRC "${SRC_DIR}/*.c" "${BUILD_SRC_DIR}/*.c")
add_executable(${PROJECT_NAME} ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC "${INCLUDE_DIR}" "${BUILD_INCLUDE_DIR}")
target_link_libraries(${PROJECT_NAME} gperf GL GLX m X11)
# ============
