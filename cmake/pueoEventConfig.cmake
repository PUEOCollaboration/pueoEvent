include("${CMAKE_CURRENT_LIST_DIR}/pueoEventTargets.cmake")
include(CMakeFindDependencyMacro)
find_dependency(pueo-data REQUIRED)
find_dependency(ROOT REQUIRED COMPONENTS TreePlayer Physics)
