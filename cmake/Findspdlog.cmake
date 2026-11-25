# Finds spdlog; falls back to FetchContent. Provides spdlog::spdlog.
include(FetchContent)

set(_bcal_spdlog_version v1.14.1)

if(NOT BCAL_FORCE_FETCH)
  find_package(spdlog CONFIG QUIET)
endif()

if(NOT TARGET spdlog::spdlog)
  message(STATUS "spdlog: fetching ${_bcal_spdlog_version}")
  set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
  set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG ${_bcal_spdlog_version}
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(spdlog)
endif()

set(spdlog_FOUND TRUE)
