cmake_minimum_required (VERSION 2.8)
project (glsl-vol)

set (INC_DIR "./include" CACHE PATH "Path to include directory")
set (LIB_DIR "./lib" CACHE PATH "Path to import library directory")
# ${IMPLIB_SOURCE_DIR}, ${IMPLIB_BINARY_DIR}

# include_directories (.)
include_directories (. ${INC_DIR})
link_directories (${LIB_DIR})

# http://www.cmake.org/cmake/help/cmake-2-8-docs.html#command:add_executable
add_executable (vol-slice volrend-slice.cpp gpumem.cpp png-util.cpp shader-util.cpp) # top level target
target_link_libraries (vol-slice png glew32 glut32 glu32 OpenGL32)