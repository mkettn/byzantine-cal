# Finds spdlog; falls back to FetchContent. Provides spdlog::spdlog.
include(FetchContent)

# 1.12.0 is the version shipped in Ubuntu 24.04 LTS. An installed copy of this
# version or newer is accepted; the same version is what gets fetched otherwise.
set(_find_spdlog_version 1.12.0)

find_package(spdlog ${_find_spdlog_version} CONFIG QUIET)

if(NOT TARGET spdlog::spdlog)
  message(STATUS "spdlog: fetching ${_find_spdlog_version}")
  set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
  set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v${_find_spdlog_version}
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(spdlog)
endif()

set(spdlog_FOUND TRUE)
