cmake_minimum_required(VERSION 3.19.0)
project(pueoEvent VERSION 1.0.0)

#================================================================================================
#                                       HOUSEKEEPING
#================================================================================================

# note: * This provides pueo-data_VERSION and GeometryReader.h 
#         (no need to link to this library just to get the header).
#       * This also imports a custom variable: PUEO_GEOMETRY_DIR
find_package(pueo-data 1.0.0 REQUIRED)  

# TODO: compiled just fine without these 
# find_package(ROOT REQUIRED COMPONENTS MathMore MathCore RIO Hist Tree Net)
# find_package(FFTW REQUIRED)
# find_package(ZLIB REQUIRED)

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/pueo/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/>
         $<INSTALL_INTERFACE:include/pueo>
)

# Propagating PUEO's version to downstream projects when they link against the pueoEvent library
# TODO: should this be a definition or an option?
target_compile_definitions(${PROJECT_NAME} PUBLIC DEFAULT_PUEO_VERSION=${pueo-data_VERSION_MAJOR}) 

# Propagating these C/C++ compiler flags to downstream projects when they link against the pueoEvent library
target_compile_options(${PROJECT_NAME} PUBLIC "-Os" "-g" "-Wall" "-Wextra")

# Propagating this linker option to downstream projects when they link against the pueoEvent library
# TODO: Not sure if this is really needed, since "-g" is already set during compilation
target_link_options(${PROJECT_NAME} PUBLIC "-g")

# TODO: compiled just fine without linking
# target_link_libraries(${PROJECT_NAME} PUEO::pueo-data ${ZLIB_LIBRARIES} RootFftwWrapper ${ROOT_LIBRARIES} ${FFTW_LIBRARIES})

# CERN ROOT dictionary generation
find_package(ROOT CONFIG REQUIRED)
set(DICTNAME G__${PROJECT_NAME})
root_generate_dictionary(${DICTNAME} ${HEADER_FILES} MODULE ${PROJECT_NAME} LINKDEF LinkDef.h)

#================================================================================================
#                                       INSTALLING
#================================================================================================

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Generate <build-directory>/pueoEventConfig.cmake from the place holder Config.cmake.in
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
)
# Generate pueoEventConfigVersion.cmake
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
# Install the generated Config and ConfigVersion files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
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
