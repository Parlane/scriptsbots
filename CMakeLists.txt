project(scriptbots)
cmake_minimum_required(VERSION 2.8)

option(USE_GPROF "Enable GProfiling" OFF)
option(USE_DEBUG "Enable Debug" OFF)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

find_package(OpenMP)

if (OPENMP_FOUND)
    SET (HAVE_OPENMP 1)
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
endif(OPENMP_FOUND)

SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -pedantic")

if (USE_DEBUG)
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
else()
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif(USE_DEBUG)

if (USE_GPROF)
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif(USE_GPROF)

include_directories(${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS} include)

set( SB_SRCS
    src/Agent.cpp
    src/AssemblyBrain.cpp
    src/Common.cpp
    src/DWRAONBrain.cpp
    src/GLView.cpp
    src/main.cpp
    src/MLPBrain.cpp
    src/View.cpp
    src/vmath.cpp
    src/World.cpp )

add_executable(scriptbots  ${SB_SRCS})

target_link_libraries(scriptbots ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})


add_custom_target(cppcheck COMMAND cppcheck "--enable=all" "--check-config" "-Iinclude" "${CMAKE_SOURCE_DIR}/src"
                           WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                           COMMENT "Running cppcheck")
