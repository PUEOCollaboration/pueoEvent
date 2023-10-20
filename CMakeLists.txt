# CMakeLists.txt for event package. It creates a library with dictionary and a main program
cmake_minimum_required(VERSION 3.0.0)
project(pueoEvent)
set(libname "pueoEvent")


# You need to tell CMake where to find the ROOT installation. This can be done in a number of ways:
#   - ROOT built with classic configure/make use the provided $ROOTSYS/etc/cmake/FindROOT.cmake
#   - ROOT built with CMake. Add in CMAKE_PREFIX_PATH the installation prefix for ROOT
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})


if(DEFINED ENV{PUEO_UTIL_INSTALL_DIR})
    message("PUEO_UTIL_INSTALL_DIR is set to $ENV{PUEO_UTIL_INSTALL_DIR}")
    set(UTIL_LIB_DIR $ENV{PUEO_UTIL_INSTALL_DIR}/lib)
    set(UTIL_INC_DIR $ENV{PUEO_UTIL_INSTALL_DIR}/include)
    set(UTIL_BIN_DIR $ENV{PUEO_UTIL_INSTALL_DIR}/bin)
    set(UTIL_SHARE_DIR $ENV{PUEO_UTIL_INSTALL_DIR}/share)
    set(LD_UTIL $ENV{PUEO_UTIL_INSTALL_DIR}/lib)
    set(INC_UTIL $ENV{PUEO_UTIL_INSTALL_DIR}/include)
else()
    message("Will install to /usr/local  PUEO_UTIL_INSTALL_DIR not set")
    set(UTIL_LIB_DIR /usr/local/lib)
    set(UTIL_INC_DIR /usr/local/include)
    set(UTIL_BIN_DIR /usr/local/bin)
    set(UTIL_SHARE_DIR /usr/local/share)
    set(LD_UTIL /usr/local/lib)
    set(INC_UTIL /usr/local/include)
endif()

set (DEFAULT_PUEO_VERSION "1" CACHE STRING "Default PUEO Version" ) 



set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${UTIL_SHARE_DIR}/cmake/modules/") 


#---Locate the ROOT package and defines a number of variables (e.g. ROOT_INCLUDE_DIRS)
find_package(ROOT REQUIRED COMPONENTS MathMore MathCore RIO Hist Tree Net)
find_package(FFTW REQUIRED)
find_package(ZLIB REQUIRED)


#---Define useful ROOT functions and macros (e.g. ROOT_GENERATE_DICTIONARY)
include(${ROOT_USE_FILE})

message("ROOT_INCLUDE_DIRS is set to ${ROOT_INCLUDE_DIRS}")
include_directories(${PROJECT_SOURCE_DIR} ${ROOT_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/src/ ${FFTW_INCLUDES} ${ZLIB_INCLUDES} ${UTIL_INC_DIR})
add_definitions(-DDEFAULT_ANITA_VERSION=${DEFAULT_ANITA_VERSION}) 

set(HEADER_FILES 
  src/pueo/Conventions.h   
  src/pueo/GeomTool.h
  src/pueo/Nav.h
  src/pueo/RawEvent.h
  src/pueo/RawHeader.h
  src/pueo/TruthEvent.h
  src/pueo/UsefulEvent.h
  src/pueo/Version.h
)

set(SOURCE_FILES 
  src/Conventions.cc   
  src/GeomTool.cc
  src/Nav.cc
  src/RawEvent.cc
  src/RawHeader.cc
  src/TruthEvent.cc
  src/UsefulEvent.cc
  src/Version.cc
)

set(DICTNAME G__${libname})

ROOT_GENERATE_DICTIONARY(${DICTNAME} ${HEADER_FILES} LINKDEF LinkDef.h)

#---Create a shared library with geneated dictionary
add_library(${libname} SHARED ${SOURCE_FILES} ${DICTNAME}.cxx)
target_link_libraries(${libname} ${ZLIB_LIBRARIES} RootFftwWrapper ${ROOT_LIBRARIES} ${FFTW_LIBRARIES})

add_custom_target(${DICTNAME}.pcm DEPENDS ${DICTNAME})


message("UTIL_INC_DIR is set to ${UTIL_INC_DIR}")
message("DICTNAME is set to ${DICTNAME}" )
message("PROJECT_BINARY_DIR is set to ${PROJECT_BINARY_DIR}")

install (FILES ${HEADER_FILES} DESTINATION ${UTIL_INC_DIR})
install (TARGETS ${libname}
	ARCHIVE DESTINATION ${UTIL_LIB_DIR}
	LIBRARY DESTINATION ${UTIL_LIB_DIR}
	RUNTIME DESTINATION ${UTIL_BIN_DIR})
install (DIRECTORY calib/ DESTINATION ${UTIL_SHARE_DIR}/anitaCalib)
      
install (FILES ${PROJECT_BINARY_DIR}/${DICTNAME}_rdict.pcm DESTINATION ${UTIL_LIB_DIR})

message (" Adding new build type") 

set(CMAKE_CXX_FLAGS_DEFAULT 
  "-Os -g -Wall -Wextra" 
  CACHE STRING "c++ Flags used during default eventReaderRoot builds" 
  FORCE ) 

set(CMAKE_C_FLAGS_DEFAULT 
  "-Os -g -Wall"
  CACHE STRING "c Flags used during default eventReaderRoot builds" 
  FORCE ) 

set(CMAKE_EXE_LINKER_FLAGS_DEFAULT 
  "-g"
  CACHE STRING "ld Flags used during default eventReaderRoot builds" 
  FORCE ) 

set(CMAKE_SHARED_LINKER_FLAGS_DEFAULT 
  "-g"
  CACHE STRING "ld Flags used during default eventReaderRoot builds" 
  FORCE ) 


mark_as_advanced ( CMAKE_CXX_FLAGS_DEFAULT CMAKE_C_FLAGS_DEFAULT CMAKE_EXE_LINKER_FLAGS_DEFAULT CMAKE_SHARED_LINKER_FLAGS_DEFAULT) 

if (NOT CMAKE_BUILD_TYPE) 
  set (CMAKE_BUILD_TYPE Default
    CACHE STRING "Choose tye type of build: None Debug Release RelWithDebInfo MinSizeRel Default"
    FORCE ) 
endif()

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

set (BINSRCDIR test) 
macro( do_binary binary_name ) 
  add_executable(${binary_name} ${BINSRCDIR}/${binary_name}.cxx) 
  target_link_libraries(${binary_name}  ${ZLIB_LIBRARIES} RootFftwWrapper ${ROOT_LIBRARIES} ${FFTW_LIBRARIES} ${libname})
  INSTALL (PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${binary_name} DESTINATION ${UTIL_BIN_DIR})
endmacro() 


