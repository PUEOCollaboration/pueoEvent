cmake_minimum_required(VERSION 3.19.0)
project(pueoEvent VERSION 1.0.0)

# note: do not glob files with wildcards
set(HEADER_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pueo/Conventions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pueo/Dataset.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pueo/GeomTool.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pueo/Nav.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pueo/RawEvent.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pueo/RawHeader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pueo/TruthEvent.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pueo/UsefulEvent.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pueo/Version.h
)
add_library(${PROJECT_NAME} SHARED 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/Conventions.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/Dataset.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/GeomTool.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/Nav.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/RawEvent.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/RawHeader.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/TruthEvent.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/UsefulEvent.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/Version.cc
)

#================================================================================================
#                                       HOUSEKEEPING
#================================================================================================
if(NOT CMAKE_BUILD_TYPE) 
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE) 
endif()

# note: * This provides pueo-data_VERSION and GeometryReader.h 
find_package(pueo-data 1.0.0 REQUIRED)  

find_package(ROOT REQUIRED COMPONENTS TreePlayer Physics)
# find_package(FFTW REQUIRED) # TODO: Pretty sure there's no need for FFTW
find_package(ZLIB REQUIRED)   # TODO: Does pueoEvent really need this?

#================================================================================================
#                                       CERN ROOT FLAGS
#================================================================================================
# note: copied from RootFftwWrapper's CERN ROOT FLAGS but with PUBLIC->PRIVATE

# obtain the C++ standard that ROOT is compiled with
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/getRootStandard.cmake)
# setting C++ standard
target_compile_features(${PROJECT_NAME} PRIVATE PUBLIC cxx_std_${ROOT_CXX_STANDARD})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

target_compile_options(${PROJECT_NAME} PRIVATE -pipe)
target_compile_options(${PROJECT_NAME} PRIVATE -fsigned-char)

#================================================================================================
#                                       BUILDING
#================================================================================================

target_include_directories(${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/>
         $<INSTALL_INTERFACE:include/pueo>
         $<INSTALL_INTERFACE:include>
)

# Propagating PUEO's version to downstream projects when they link against the pueoEvent library
# TODO: let CMake communicate the version to the source code?
target_compile_definitions(${PROJECT_NAME} PUBLIC DEFAULT_PUEO_VERSION=${pueo-data_VERSION_MAJOR}) 

# use warning flags in RelWithDebInfo mode, which is set to be the default mode
target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:RelWithDebInfo>:-Wall -Wextra>)

target_link_libraries(${PROJECT_NAME} 
  PUBLIC  PUEO::pueo-data 
  PRIVATE ZLIB::ZLIB ROOT::TreePlayer ROOT::Physics
)

# CERN ROOT dictionary generation
set(DICTNAME G__${PROJECT_NAME})
root_generate_dictionary(${DICTNAME} ${HEADER_FILES} MODULE ${PROJECT_NAME} LINKDEF LinkDef.h)

#================================================================================================
#                                       INSTALLING
#================================================================================================

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Generate pueoEventConfigVersion.cmake
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
# Install the Config and ConfigVersion files
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION lib/cmake/${PROJECT_NAME}
)

# Install the header files (ie. this copies ./src/pueo/* to <prefix>/include/pueo/*)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/pueo DESTINATION include) 

# note: not installing the library to lib64 for backward compatibility
install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Targets LIBRARY DESTINATION lib )

# installing CERN ROOT's _rdict.pcm file
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}_rdict.pcm"
  DESTINATION DESTINATION lib
)

install(EXPORT ${PROJECT_NAME}Targets
        NAMESPACE PUEO::
        FILE ${PROJECT_NAME}Targets.cmake
        DESTINATION lib/cmake/${PROJECT_NAME}
)
