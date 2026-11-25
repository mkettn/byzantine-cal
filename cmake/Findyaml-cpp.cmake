# Finds yaml-cpp; falls back to FetchContent. Provides yaml-cpp::yaml-cpp.
include(FetchContent)

set(_bcal_yaml_version 0.8.0)

if(NOT BCAL_FORCE_FETCH)
  find_package(yaml-cpp ${_bcal_yaml_version} CONFIG QUIET)
endif()

if(NOT TARGET yaml-cpp::yaml-cpp)
  message(STATUS "yaml-cpp: fetching ${_bcal_yaml_version}")
  # Old dependencies declare cmake_minimum_required < 3.5 (rejected by CMake 4).
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
  set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
  set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
  set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG ${_bcal_yaml_version}
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(yaml-cpp)
  if(NOT TARGET yaml-cpp::yaml-cpp)
    add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
  endif()
endif()

set(yaml-cpp_FOUND TRUE)
