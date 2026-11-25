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
  set(RUNNER ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_expect.cmake)
  function(bcal_expect name file code regex)
    add_test(NAME ${name}
             COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:byzantine-cal>
                     -DARGS=${CMAKE_CURRENT_SOURCE_DIR}/tests/${file}
                     -DCODE=${code} "-DREGEX=${regex}" -P ${RUNNER})
  endfunction()

  file(GLOB valid_files CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/valid/*.yml)
  foreach(f ${valid_files})
    get_filename_component(n ${f} NAME_WE)
    bcal_expect(valid_${n} valid/${n}.yml 0 "OK")
  endforeach()

  bcal_expect(invalid_bad_day invalid/bad_day.yml 1 "day 31 does not exist")
  bcal_expect(invalid_old_syntax invalid/old_syntax.yml 1 "expected DD.MM")
  bcal_expect(invalid_bad_range_end invalid/bad_range_end.yml 1 "does not exist")
  bcal_expect(invalid_range_no_rules invalid/range_no_rules.yml 1 "needs weekday rules")
  bcal_expect(invalid_empty_range invalid/empty_range.yml 1 "no weekday rules")
  bcal_expect(invalid_bad_weekday invalid/bad_weekday.yml 1 "unknown weekday")
  bcal_expect(invalid_bad_rule invalid/bad_rule.yml 1 "unknown rule")
  bcal_expect(invalid_dup_weekday invalid/dup_weekday.yml 1 "listed twice")
  bcal_expect(invalid_mixed_weekday invalid/mixed_weekday.yml 1 "invalid weekday range")
  bcal_expect(invalid_upper_weekday invalid/upper_weekday.yml 1 "lowercase")
  bcal_expect(invalid_empty_name invalid/empty_name.yml 1 "empty name")
  bcal_expect(invalid_null_name invalid/null_name.yml 1 "non-empty text")
  bcal_expect(invalid_easter_backwards invalid/easter_backwards.yml 1 "ends before it starts")
  bcal_expect(invalid_empty_file invalid/empty_file.yml 1 "file is empty")
  bcal_expect(invalid_multi_doc invalid/multi_doc.yml 1 "YAML documents")
  bcal_expect(invalid_not_a_list invalid/not_a_list.yml 1 "must be a list")
  bcal_expect(invalid_yaml_syntax invalid/yaml_syntax.yml 1 "YAML syntax error")
  bcal_expect(error_missing_file invalid/nope.yml 2 "cannot read")
  bcal_expect(error_directory invalid 2 "cannot read")
  add_test(NAME error_usage COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:byzantine-cal>
           -DCODE=3 -P ${RUNNER})
endif()
