cmake_minimum_required(VERSION 3.8.0)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(ubitrack_vision VERSION 1.3.0 LANGUAGES C CXX)

include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

include(cmake/UbitrackConfig)


# External Libraries (could this be solved with conan dependencies ??)

# OpenGL
SET(HAVE_OPENGL 0)
find_package(OpenGL)
IF(OpenGL_FOUND)
  SET(HAVE_OPENGL 1)
  MESSAGE(STATUS "Found OpenGL: ${OpenGL_INCLUDE_DIR} - ${OpenGL_LIBRARIES}")
ENDIF(OpenGL_FOUND)

# OpenCL
SET(HAVE_OPENCL 0)
find_package(OpenCL)
IF(OpenCL_FOUND)
  include_directories(${OpenCL_INCLUDE_DIR})
  SET(HAVE_OPENCL 1)
  MESSAGE(STATUS "Found OpenCL: ${OpenCL_INCLUDE_DIR} - ${OpenCL_LIBRARY}")

  if(UNIX AND NOT APPLE)
    include(CheckOpenCLICDLoader)
    if(OpenCL_C_WORKS AND NOT OpenCL_CXX_WORKS)
      set(OPENCL_ICD_LOADER_IS_OLD 1)
      message(WARNING "Your libOpenCL.so is incompatible with CL/cl.h. Install ocl-icd-opencl-dev to update libOpenCL.so?")
    endif()
  endif()

ENDIF(OpenCL_FOUND)

# we can safely assume that these libraries are available.
SET(HAVE_OPENCV 1)
SET(HAVE_LAPACK 1)

add_subdirectory(src)

if(ENABLE_UNITTESTS)
  add_subdirectory(tests)
endif(ENABLE_UNITTESTS)
