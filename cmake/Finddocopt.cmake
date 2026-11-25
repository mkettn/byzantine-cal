# Finds docopt.cpp; falls back to FetchContent. Provides target docopt_s.
include(FetchContent)

set(_bcal_docopt_version v0.6.3)

if(NOT BCAL_FORCE_FETCH)
  find_package(docopt CONFIG QUIET)
endif()

if(NOT TARGET docopt_s AND NOT TARGET docopt)
  message(STATUS "docopt: fetching ${_bcal_docopt_version}")
  # Old dependencies declare cmake_minimum_required < 3.5 (rejected by CMake 4).
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
  set(WITH_TESTS OFF CACHE BOOL "" FORCE)
  set(WITH_STATIC ON CACHE BOOL "" FORCE)
  FetchContent_Declare(docopt
    GIT_REPOSITORY https://github.com/docopt/docopt.cpp.git
    GIT_TAG ${_bcal_docopt_version}
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(docopt)
endif()

# Installed package exports "docopt"; normalise to docopt_s for consumers.
if(NOT TARGET docopt_s AND TARGET docopt)
  add_library(docopt_s INTERFACE)
  target_link_libraries(docopt_s INTERFACE docopt)
endif()

set(docopt_FOUND TRUE)
