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
find_package(nlohmann_json REQUIRED)
find_package(inja REQUIRED)

# The templates (tpl/) are installed to <prefix>/lib/byzantine-cal/tpl and read from
# there at run time. The prefix is CMAKE_INSTALL_PREFIX (default /usr/local); set it
# when configuring, e.g. -DCMAKE_INSTALL_PREFIX=/usr.
set(BCAL_TPL_DIR "${CMAKE_INSTALL_PREFIX}/lib/byzantine-cal/tpl")

add_library(bcal STATIC src/day.cpp src/validator.cpp src/pascha.cpp
            src/year_calendar.cpp src/calendar_loader.cpp
            src/template_backend.cpp)
target_include_directories(bcal PUBLIC src)
target_link_libraries(bcal PUBLIC yaml-cpp::yaml-cpp spdlog::spdlog
                      nlohmann_json::nlohmann_json pantor::inja)
target_compile_options(bcal PRIVATE -Wall -Wextra -Wpedantic)

add_executable(byzantine-cal src/main.cpp)
target_link_libraries(byzantine-cal PRIVATE bcal docopt_s)
target_compile_definitions(byzantine-cal PRIVATE BCAL_VERSION="${PROJECT_VERSION}"
                           BCAL_TPL_DIR="${BCAL_TPL_DIR}")
target_compile_options(byzantine-cal PRIVATE -Wall -Wextra -Wpedantic)

install(TARGETS byzantine-cal RUNTIME DESTINATION bin)
install(DIRECTORY tpl/ DESTINATION lib/byzantine-cal/tpl)

include(CTest)
if(BUILD_TESTING)
  add_executable(bcal-tests tests/test_main.cpp)
  target_link_libraries(bcal-tests PRIVATE bcal)
  add_test(NAME unit COMMAND bcal-tests)
  set(RUNNER ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_expect.cmake)
  # Arguments separated by "|"; exit code and a regex on the output are checked.
  function(bcal_expect_args name args code regex)
    add_test(NAME ${name}
             COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:byzantine-cal>
                     "-DARGS=${args}" -DCODE=${code} "-DREGEX=${regex}" -P ${RUNNER})
  endfunction()
  function(bcal_expect name file code regex)
    bcal_expect_args(${name} ${CMAKE_CURRENT_SOURCE_DIR}/tests/${file} ${code} "${regex}")
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
  bcal_expect(invalid_mixed_weekday invalid/mixed_weekday.yml 1 "invalid weekday range")
  bcal_expect(invalid_upper_weekday invalid/upper_weekday.yml 1 "lowercase")
  bcal_expect(invalid_empty_name invalid/empty_name.yml 1 "empty name")
  bcal_expect(invalid_null_name invalid/null_name.yml 1 "non-empty text")
  bcal_expect(invalid_pascha_backwards invalid/pascha_backwards.yml 1 "ends before it starts")
  bcal_expect(invalid_empty_file invalid/empty_file.yml 1 "file is empty")
  bcal_expect(invalid_multi_doc invalid/multi_doc.yml 1 "YAML documents")
  bcal_expect(invalid_not_a_list invalid/not_a_list.yml 1 "must be a list")
  bcal_expect(invalid_yaml_syntax invalid/yaml_syntax.yml 1 "YAML syntax error")
  bcal_expect(error_missing_file invalid/nope.yml 2 "cannot read")
  bcal_expect(error_directory invalid 2 "cannot read")
  set(NATIVITY ${CMAKE_CURRENT_SOURCE_DIR}/tests/year/nativity.yml)
  # The built tree has no installed templates, so the tests name the Markdown one.
  set(MD --template=${CMAKE_CURRENT_SOURCE_DIR}/tpl/markdown.md.inja)
  bcal_expect_args(year_plain "--year=2026|${MD}|${NATIVITY}" 0 "2026-12-25.+Nativity")
  bcal_expect_args(year_old "--year=2026|--old|${MD}|${NATIVITY}" 0 "2026-01-07.+Nativity")
  bcal_expect_args(year_old_pascha "--year=2026|--old|${MD}|${NATIVITY}" 0 "2026-04-12.+Pascha")
  bcal_expect_args(year_data "--year=2026|${MD}|${NATIVITY}" 0 "2026-11-16.+note: fast")
  set(TPL ${CMAKE_CURRENT_SOURCE_DIR}/tests/templates)
  bcal_expect_args(year_markdown "--year=2026|${MD}|${NATIVITY}" 0 "# 2026.+Pascha: 2026-04-12.+## April.+2026-04-12 \\| Sunday")
  bcal_expect_args(template_file "--year=2026|--old|--template=${TPL}/list.txt.inja|${NATIVITY}" 0 "2026-01-07;3;{name: Nativity")
  bcal_expect_args(template_broken "--year=2026|--template=${TPL}/broken.inja|${NATIVITY}" 1 "template error")
  bcal_expect_args(template_unknown_value "--year=2026|--template=${TPL}/unknown.inja|${NATIVITY}" 1 "template error")
  bcal_expect_args(template_missing "--year=2026|--template=${TPL}/nope.inja|${NATIVITY}" 2 "cannot read template")
  bcal_expect_args(template_without_year "--template=${TPL}/list.txt.inja|${NATIVITY}" 3 "need --year")
  # Without --template the Markdown template is read from the install location.
  add_test(NAME default_template
           COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:byzantine-cal> -DTPL_DIR=${BCAL_TPL_DIR}
                   -DYAML=${NATIVITY} -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/default_template.cmake)
  add_test(NAME output_file
           COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:byzantine-cal>
                   "-DARGS=--year=2026|${MD}|--output=${CMAKE_CURRENT_BINARY_DIR}/out.md|${NATIVITY}"
                   -DCODE=0 "-DOUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/out.md"
                   "-DOUT_REGEX=# 2026.+Nativity" -P ${RUNNER})
  bcal_expect_args(output_file_unwritable "--year=2026|${MD}|--output=${CMAKE_CURRENT_BINARY_DIR}/no/such/dir/out.md|${NATIVITY}" 2 "cannot write")
  bcal_expect_args(year_not_a_number "--year=abc|${NATIVITY}" 3 "whole number")
  bcal_expect_args(year_out_of_range "--year=0|${NATIVITY}" 3 "1..9999")
  bcal_expect_args(old_without_year "--old|${NATIVITY}" 3 "need --year")
  bcal_expect_args(year_invalid_file "--year=2026|${CMAKE_CURRENT_SOURCE_DIR}/tests/invalid/bad_day.yml" 1 "does not exist")
  add_test(NAME error_usage COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:byzantine-cal>
           -DCODE=3 -P ${RUNNER})
endif()
