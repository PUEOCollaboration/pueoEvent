cmake_minimum_required(VERSION 3.19.0)
project(pueoEvent VERSION 1.0.0)

file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

#================================================================================================
#                                       HOUSEKEEPING
#================================================================================================

# note: * This provides pueo-data_VERSION and GeometryReader.h 
find_package(pueo-data 1.0.0 REQUIRED)  

find_package(ROOT REQUIRED COMPONENTS TreePlayer Physics)
# find_package(FFTW REQUIRED) # TODO: Pretty sure there's no need for FFTW
find_package(ZLIB REQUIRED)   # TODO: Does pueoEvent really need this?

#================================================================================================
#                                       BUILDING
#================================================================================================

target_include_directories(${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/>
         $<INSTALL_INTERFACE:include/pueo>
)

# Transform ROOT_CXX_FLAGS into a list of individual flags, and add them to the compilation command
separate_arguments( ROOT_CXX_FLAGS_LIST UNIX_COMMAND ${ROOT_CXX_FLAGS})
# adding these flags privately, since downstream projects would have gotten these flags already from
# RootFftwWrapper
target_compile_options(${PROJECT_NAME} PRIVATE ${ROOT_CXX_FLAGS_LIST})

# Propagating PUEO's version to downstream projects when they link against the pueoEvent library
# TODO: let CMake communicate the version to the source code?
target_compile_definitions(${PROJECT_NAME} PUBLIC DEFAULT_PUEO_VERSION=${pueo-data_VERSION_MAJOR}) 

# use warning flags only in debug mode
target_compile_options(${PROJECT_NAME} PRIVATE
  $<$<CONFIG:Debug>:-Wall -Wextra>
)

target_link_libraries(${PROJECT_NAME} 
  PUBLIC  PUEO::pueo-data 
  PRIVATE ZLIB::ZLIB ROOT::TreePlayer ROOT::Physics
)

# CERN ROOT dictionary generation
file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/pueo/*.h")
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
# TODO: not sure if ROOT .pcm files should still be installed (if so, then install to <prefix>/lib

install(EXPORT ${PROJECT_NAME}Targets
        NAMESPACE PUEO::
        FILE ${PROJECT_NAME}Targets.cmake
        DESTINATION lib/cmake/${PROJECT_NAME}
)
