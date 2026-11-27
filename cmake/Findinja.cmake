# Finds inja (header-only template engine); falls back to FetchContent.
# Provides pantor::inja. Uses the nlohmann_json found by Findnlohmann_json.cmake.
include(FetchContent)

# Ubuntu 24.04 LTS does not ship inja; 3.4.0 is the release that matches the
# nlohmann_json 3.11 shipped there.
set(_find_inja_version 3.4.0)

find_package(nlohmann_json REQUIRED)
find_package(inja ${_find_inja_version} CONFIG QUIET)

if(NOT TARGET pantor::inja)
  message(STATUS "inja: fetching ${_find_inja_version}")
  set(INJA_USE_EMBEDDED_JSON OFF CACHE BOOL "" FORCE)
  set(INJA_INSTALL OFF CACHE BOOL "" FORCE)
  set(INJA_EXPORT OFF CACHE BOOL "" FORCE)
  set(INJA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(inja
    GIT_REPOSITORY https://github.com/pantor/inja.git
    GIT_TAG v${_find_inja_version}
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(inja)
endif()

set(inja_FOUND TRUE)
