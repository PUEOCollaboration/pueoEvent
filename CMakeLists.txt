cmake_minimum_required(VERSION 3.23.0)
project(pueoEvent VERSION 1.0.0)
add_library(${PROJECT_NAME} SHARED)
include(GNUInstallDirs)


set(HEADER_FILES
  src/pueo/Conventions.h
  src/pueo/Converter.h
  src/pueo/Dataset.h
  src/pueo/GeomTool.h
  src/pueo/Nav.h
  src/pueo/RawEvent.h
  src/pueo/RawHeader.h
  src/pueo/TruthEvent.h
  src/pueo/UsefulEvent.h
  src/pueo/Version.h
)
target_sources(${PROJECT_NAME} PRIVATE
  src/Conventions.cc
  src/Converter.cc
  src/Dataset.cc
  src/GeomTool.cc
  src/Nav.cc
  src/RawEvent.cc
  src/RawHeader.cc
  src/TruthEvent.cc
  src/UsefulEvent.cc
  src/Version.cc
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "This creates compile_commands.json, useful for LSP" FORCE)

#================================================================================================
#                                       HOUSEKEEPING
#================================================================================================
if(NOT CMAKE_BUILD_TYPE) 
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE) 
endif()

#look for libpueorawdata, and enable converting if it's found
find_package(pueorawdata CONFIG)

# note: * This provides pueo-data_VERSION and GeometryReader.h 
find_package(pueo-data 1.0.0 REQUIRED)  

find_package(ROOT REQUIRED COMPONENTS TreePlayer Physics)

#================================================================================================
#                                       CERN ROOT C++ Standard
#================================================================================================
# note: copied from RootFftwWrapper's CERN ROOT FLAGS but with PUBLIC->PRIVATE

# obtain the C++ standard that ROOT is compiled with
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/getRootStandard.cmake)
# setting C++ standard accordingly for this project
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_${ROOT_CXX_STANDARD})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)

#================================================================================================
#                                       BUILDING
#================================================================================================

target_sources(${PROJECT_NAME} PUBLIC
  FILE_SET HEADERS
  BASE_DIRS src          # <-- Everything in BASE_DIRS is available during the build
  FILES ${HEADER_FILES}  # <-- But only FILES will be installed, although these two are currently the same
)

# Propagating PUEO's version to downstream projects when they link against the pueoEvent library
# TODO: let CMake communicate the version to the source code?
target_compile_definitions(${PROJECT_NAME} PUBLIC DEFAULT_PUEO_VERSION=${pueo-data_VERSION_MAJOR}) 

# use warning flags in RelWithDebInfo mode, which is set to be the default mode
target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:RelWithDebInfo>:-Wall -Wextra>)

target_link_libraries(${PROJECT_NAME} 
  PUBLIC  PUEO::pueo-data ROOT::TreePlayer ROOT::Physics
)

if (pueorawdata_FOUND)
  message(STATUS "Found libpueorawdata")
  target_compile_options(${PROJECT_NAME} PRIVATE -DHAVE_PUEORAWDATA)
  target_link_libraries(${PROJECT_NAME} PRIVATE PUEO::pueorawdata)

  add_executable(pueo-convert src/pueo-convert.cc)
  target_link_libraries(pueo-convert ${PROJECT_NAME})
  install(
    TARGETS pueo-convert
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

# CERN ROOT dictionary generation
set(DICTNAME G__${PROJECT_NAME})
root_generate_dictionary(${DICTNAME} ${HEADER_FILES} MODULE ${PROJECT_NAME} LINKDEF LinkDef.h)

#================================================================================================
#                                       INSTALLING
#================================================================================================

# Generate pueoEventConfigVersion.cmake
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Install the Config and ConfigVersion files; the config file tells downstream projects how to find
# the Targets.config file of this project, as well as how to find the dependencies of this project
install(
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
)

# Install the library and its header files and group all that into one export
install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets 
  FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  LIBRARY DESTINATION lib
)

install(PROGRAMS scripts/pueo-convert-run.sh DESTINATION bin RENAME pueo-convert-run)

# generate a Targets.config from the export; install the file dirrectly to the install path
# this file lets downstream projects know how to interact with the package that is this project.
install(
  EXPORT ${PROJECT_NAME}Targets
  NAMESPACE PUEO::
  DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
)

# installing CERN ROOT's _rdict.pcm file
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}_rdict.pcm"
  DESTINATION lib
)

