# Finds yaml-cpp; falls back to FetchContent. Provides yaml-cpp::yaml-cpp.
include(FetchContent)

# 0.8.0 is the version shipped in Ubuntu 24.04 LTS. An installed copy of this
# version or newer is accepted; the same version is what gets fetched otherwise.
set(_find_yaml_cpp_version 0.8.0)

find_package(yaml-cpp ${_find_yaml_cpp_version} CONFIG QUIET)

if(NOT TARGET yaml-cpp::yaml-cpp)
  message(STATUS "yaml-cpp: fetching ${_find_yaml_cpp_version}")
  # Old dependencies declare cmake_minimum_required < 3.5 (rejected by CMake 4).
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
  set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
  set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
  set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG ${_find_yaml_cpp_version}
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(yaml-cpp)
  if(NOT TARGET yaml-cpp::yaml-cpp)
    add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
  endif()
endif()

set(yaml-cpp_FOUND TRUE)
