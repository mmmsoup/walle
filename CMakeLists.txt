project(walle)

cmake_minimum_required(VERSION 3.13)

# === DIRS ===
set(CMAKE_CURRENT_BINARY_DIR "${CMAKE_CURRENT_LIST_DIR}/build")
set(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src")
set(INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(BUILD_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/src)
set(BUILD_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
# ============

# == CONFIG ==
configure_file("${CMAKE_CURRENT_LIST_DIR}/config.h.in" "${BUILD_INCLUDE_DIR}/config.h")
# ============

# == CFLAGS ==
# modified from https://cmake.org/pipermail/cmake/2013-January/053117.html
set(FILENAME_MACRO "-D__FILENAME__='\"$(patsubst CMakeFiles/${PROJECT_NAME}.dir/%.o,%,$@)\"'")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FILENAME_MACRO}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${FILENAME_MACRO} -DENABLE_DEBUG -Wall -Wno-unused-variable -ggdb")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} ${FILENAME_MACRO} -O3")
# ============

# === GL3W ===
set(GL3W_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/gl3w")
execute_process(COMMAND python gl3w_gen.py --root "${CMAKE_CURRENT_BINARY_DIR}" WORKING_DIRECTORY "${GL3W_SRC_DIR}")
# ============

# === MAIN ===
file(GLOB SRC "${SRC_DIR}/*.c" "${BUILD_SRC_DIR}/*.c")
add_executable(${PROJECT_NAME} ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC "${INCLUDE_DIR}" "${BUILD_INCLUDE_DIR}")
target_link_libraries(${PROJECT_NAME} GL GLX m X11)
# ============
