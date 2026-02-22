include("${CMAKE_CURRENT_LIST_DIR}/pueoEventTargets.cmake")
include(CMakeFindDependencyMacro)
find_dependency(pueo-data)
find_dependency(ROOT REQUIRED COMPONENTS TreePlayer Physics)
find_package(pueorawdata) # optional

