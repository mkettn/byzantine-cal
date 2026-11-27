# Finds nlohmann_json; falls back to FetchContent. Provides nlohmann_json::nlohmann_json.
include(FetchContent)

# 3.11.3 is the version shipped in Ubuntu 24.04 LTS. An installed copy of this
# version or newer is accepted; the same version is what gets fetched otherwise.
set(_find_nlohmann_json_version 3.11.3)

find_package(nlohmann_json ${_find_nlohmann_json_version} CONFIG QUIET)

if(NOT TARGET nlohmann_json::nlohmann_json)
  message(STATUS "nlohmann_json: fetching ${_find_nlohmann_json_version}")
  set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
  set(JSON_Install OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v${_find_nlohmann_json_version}
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(nlohmann_json)
endif()

set(nlohmann_json_FOUND TRUE)
