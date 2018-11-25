set(UBITRACK_LIBRARY_NAME utvision)


# writes config file into binary_dir, so sourcetree will not be cluttered
configure_file(utVision/Config_cmake.hh ${CMAKE_CURRENT_SOURCE_DIR}/utVision/Config.h)

# Source and Header Files

# All cpp files for UTCORE
FILE(GLOB UBITRACK_SRCS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
   "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/*/*/*.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/*/*/*/*.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/*/*/*/*/*.cpp"
)

# All header files for UTCORE
FILE(GLOB UBITRACK_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
   "${CMAKE_CURRENT_SOURCE_DIR}/*.h"  
   "${CMAKE_CURRENT_SOURCE_DIR}/*/*.h" 
   "${CMAKE_CURRENT_SOURCE_DIR}/*/*/*.h"
   "${CMAKE_CURRENT_SOURCE_DIR}/*/*/*/*.h"
   "${CMAKE_CURRENT_SOURCE_DIR}/*/*/*/*/*.h"
)

# UTCORE library definition
ADD_LIBRARY(${UBITRACK_LIBRARY_NAME} ${UBITRACK_SRCS} ${UBITRACK_HEADERS})

if(HAVE_GLAD)
  set(GLWRAPPER_TARGET CONAN_PKG::glad)
elseif(HAVE_GLEW)
  set(GLWRAPPER_TARGET CONAN_PKG::glew)
endif(HAVE_GLAD)

target_link_libraries(${UBITRACK_LIBRARY_NAME} 
  CONAN_PKG::ubitrack_core
  CONAN_PKG::opencv
  CONAN_PKG::glext
  ${GLWRAPPER_TARGET}
  OpenGL::GL
  OpenGL::GLU
  OpenCL::OpenCL
  )

target_include_directories(${UBITRACK_LIBRARY_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
  PRIVATE src)

if(HAVE_GLAD)
    set(UBITRACK_COMPILE_DEFINITIONS ${UBITRACK_COMPILE_DEFINITIONS} "HAVE_GLAD")
endif(HAVE_GLAD)

if(WIN32)
  IF(BUILD_SHARED_LIBS)
    string(TOUPPER "${UBITRACK_LIBRARY_NAME}" _libname_upper)
    set(UBITRACK_COMPILE_DEFINITIONS ${UBITRACK_COMPILE_DEFINITIONS} "${_libname_upper}_DLL")
  ENDIF(BUILD_SHARED_LIBS)
endif(WIN32)

# Apply Target Properties
if(MSVC)
  if(CMAKE_CROSSCOMPILING)
    set_target_properties(${UBITRACK_LIBRARY_NAME} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:secchk")
  endif()
  set_target_properties(${UBITRACK_LIBRARY_NAME} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:libc /DEBUG")
endif()

ubitrack_set_target_properties(${UBITRACK_LIBRARY_NAME})
ubitrack_install_headers(${UBITRACK_HEADERS})
ubitrack_install_library(${UBITRACK_LIBRARY_NAME})
