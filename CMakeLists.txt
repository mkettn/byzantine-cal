cmake_minimum_required(VERSION 3.24)
project(byzantine-cal VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies are provided by cmake/Find<Lib>.cmake (FetchContent based).
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(yaml-cpp REQUIRED)
find_package(spdlog REQUIRED)
find_package(docopt REQUIRED)

add_library(bcal STATIC src/day.cpp src/validator.cpp)
target_include_directories(bcal PUBLIC src)
target_link_libraries(bcal PUBLIC yaml-cpp::yaml-cpp spdlog::spdlog)
target_compile_options(bcal PRIVATE -Wall -Wextra -Wpedantic)

add_executable(byzantine-cal src/main.cpp)
target_link_libraries(byzantine-cal PRIVATE bcal docopt_s)
target_compile_definitions(byzantine-cal PRIVATE BCAL_VERSION="${PROJECT_VERSION}")
target_compile_options(byzantine-cal PRIVATE -Wall -Wextra -Wpedantic)

include(CTest)
if(BUILD_TESTING)
  add_executable(bcal-tests tests/test_main.cpp)
  target_link_libraries(bcal-tests PRIVATE bcal)
  add_test(NAME unit COMMAND bcal-tests)
  add_test(NAME valid_example
           COMMAND byzantine-cal ${CMAKE_CURRENT_SOURCE_DIR}/tests/valid.yml)
  add_test(NAME invalid_example
           COMMAND byzantine-cal ${CMAKE_CURRENT_SOURCE_DIR}/tests/invalid.yml)
  set_tests_properties(invalid_example PROPERTIES WILL_FAIL TRUE)
endif()
