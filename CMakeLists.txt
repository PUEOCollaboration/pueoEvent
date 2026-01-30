cmake_minimum_required(VERSION 3.23.0)
project(pueoEvent VERSION 1.0.0)
add_library(${PROJECT_NAME} SHARED)



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




#================================================================================================
#                                       HOUSEKEEPING
#================================================================================================
if(NOT CMAKE_BUILD_TYPE) 
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE) 
endif()

#look for libpueorawdata, and enable converting if it's found
find_library(PUEORAWDATA NAMES pueorawdata libpueorawdata PATHS "$ENV{PUEO_UTIL_INSTALL_DIR}/lib")


# note: * This provides pueo-data_VERSION and GeometryReader.h 
find_package(pueo-data 1.0.0 REQUIRED)  

find_package(ROOT REQUIRED COMPONENTS TreePlayer Physics)
# find_package(FFTW REQUIRED) # TODO: Pretty sure there's no need for FFTW
find_package(ZLIB REQUIRED)   # TODO: Does pueoEvent really need this?



if (PUEORAWDATA)
  target_compile_options(${PROJECT_NAME} PRIVATE -DHAVE_PUEORAWDATA)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${PUEORAWDATA})
  message("Found libpueorawdata")
else()
  message("No libpueorawdata found, will not be able to convert")

endif()
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
  PUBLIC  PUEO::pueo-data 
  PRIVATE ZLIB::ZLIB ROOT::TreePlayer ROOT::Physics
)

add_executable(pueo-convert src/pueo-convert.cc)
target_link_libraries(pueo-convert ${PROJECT_NAME})

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
# Install the Config and ConfigVersion files
include(GNUInstallDirs)
install(
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
)

# Install the library and its header files
install(
  TARGETS ${PROJECT_NAME} pueo-convert
  EXPORT ${PROJECT_NAME}Targets 
  FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  EXPORT ${PROJECT_NAME}Targets
  NAMESPACE PUEO::
  DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
)

# installing CERN ROOT's _rdict.pcm file
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}_rdict.pcm"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

