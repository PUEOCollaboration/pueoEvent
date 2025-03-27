# File: getRootStandard.cmake
# Purpose: Returns ROOT_CXX_STANDARD if the flag -std=c++17 (or whatever other number) is present in 
#          ${ROOT_CXX_FLAGS}; if ROOT_CXX_STANDARD is already populated, then the standard is simply
#          printed out.
# Resources:
#   1) code slightly modified from https://root-forum.cern.ch/t/getting-the-c-standard-if-root-cxx-standard-is-not-set/59439
#   2) original post: https://root-forum.cern.ch/t/cmake-variable-root-cxx-standard-not-set/59207/3
#   3) related posts: https://root-forum.cern.ch/t/compiler-flag-errors/61908/5
#   4)                https://root-forum.cern.ch/t/cmake-cxx-std-propagation/62623/2
#

# some nice colours
if(NOT WIN32)
  string(ASCII 27 Esc)
  set(ColourReset "${Esc}[m")
  set(BoldGreen   "${Esc}[1;32m")
endif()


if(NOT ROOT_CXX_STANDARD)
  string(REGEX REPLACE ".*-std=.+\\+\\+([0-9][0-9]).*" "\\1" ROOT_CXX_STANDARD ${ROOT_CXX_FLAGS})
  if(${CMAKE_MATCH_COUNT} EQUAL "1")
    message(STATUS "${BoldGreen}Got ROOT_CXX_STANDARD to be ${ROOT_CXX_STANDARD} from ROOT_CXX_FLAGS (${ROOT_CXX_FLAGS})${ColourReset}")
  else()
    message(FATAL_ERROR 
      "The ROOT cxx flags seem to be misformed (${ROOT_CXX_FLAGS}) (CMAKE_MATCH_COUNT=${CMAKE_MATCH_COUNT}), 
      consider providing a cxx version by hand using \"-DROOT_CXX_VERSION=??\"")
  endif()
else()
  message(STATUS "${BoldGreen}Got ROOT_CXX_STANDARD: ${ROOT_CXX_STANDARD}${ColourReset}")
endif()
