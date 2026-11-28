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

# The installed config.yaml is looked for under the install prefix (see the README).
set(BCAL_CONFIG_FILE "${CMAKE_INSTALL_PREFIX}/lib/byzantine-cal/config.yaml")

add_library(bcal STATIC src/day.cpp src/validator.cpp src/pascha.cpp
            src/year_calendar.cpp src/calendar_loader.cpp
            src/template_backend.cpp src/byzantine_cal_config.cpp
            src/language.cpp)
target_include_directories(bcal PUBLIC src)
target_link_libraries(bcal PUBLIC yaml-cpp::yaml-cpp spdlog::spdlog
                      nlohmann_json::nlohmann_json pantor::inja)
target_compile_options(bcal PRIVATE -Wall -Wextra -Wpedantic)

add_executable(byzantine-cal src/main.cpp)
target_link_libraries(byzantine-cal PRIVATE bcal docopt_s)
target_compile_definitions(byzantine-cal PRIVATE BCAL_VERSION="${PROJECT_VERSION}"
                           BCAL_CONFIG_FILE="${BCAL_CONFIG_FILE}")
target_compile_options(byzantine-cal PRIVATE -Wall -Wextra -Wpedantic)

install(TARGETS byzantine-cal RUNTIME DESTINATION bin)
install(FILES config/config.yaml DESTINATION lib/byzantine-cal)
install(DIRECTORY tpl/ DESTINATION lib/byzantine-cal/tpl)
install(DIRECTORY lang/ DESTINATION lib/byzantine-cal/lang)

include(CTest)
if(BUILD_TESTING)
  add_executable(bcal-tests tests/test_main.cpp)
  target_link_libraries(bcal-tests PRIVATE bcal)
  target_compile_definitions(bcal-tests PRIVATE
                             BCAL_TPL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tpl"
                             BCAL_TEST_TPL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/templates"
                             BCAL_TEST_CONFIG="${CMAKE_CURRENT_SOURCE_DIR}/tests/config/bcal.yaml")
  add_test(NAME unit COMMAND bcal-tests)
  set(RUNNER ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_expect.cmake)
  # Arguments separated by "|"; exit code and a regex on the output are checked.
  # An optional last argument is the working directory.
  function(bcal_expect_args name args code regex)
    add_test(NAME ${name}
             COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:byzantine-cal>
                     "-DARGS=${args}" -DCODE=${code} "-DREGEX=${regex}" -P ${RUNNER})
    if(ARGN)
      set_tests_properties(${name} PROPERTIES WORKING_DIRECTORY ${ARGN})
    endif()
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
  set(CFG ${CMAKE_CURRENT_SOURCE_DIR}/tests/config)
  bcal_expect_args(year_old "--year=2026|--old|-C|${CFG}/bcal.yaml|${NATIVITY}" 0 "2026-01-07.+Nativity")
  bcal_expect_args(year_data "--year=2026|-C|${CFG}/bcal.yaml|${NATIVITY}" 0 "2026-11-16.+note: fast")
  bcal_expect_args(year_markdown "--year=2026|-C|${CFG}/bcal.yaml|${NATIVITY}" 0 "# 2026.+Pascha: 2026-04-12.+## April.+2026-04-12 \\| Sunday")
  bcal_expect_args(year_markdown_escaped "--year=2026|-C|${CFG}/bcal.yaml|${CMAKE_CURRENT_SOURCE_DIR}/tests/year/special.yml" 0
                   "Theophany\\\\_Feast.+has \\\\`tick\\\\` \\\\\\| pipe")
  # Languages. 7 January 2026 is a Wednesday: with --old the Nativity fast ends on it.
  # The configuration says en_US; --lang=<name> uses the file <name>.yaml in a lang_dir.
  set(OLD "--year=2026|--old|-C|${CFG}/bcal.yaml")
  bcal_expect_args(lang_english "${OLD}|${NATIVITY}" 0 "Pascha: 2026-04-12.+## January.+\\| Date \\| Weekday \\| Data \\|.+2026-01-07 \\| Wednesday")
  bcal_expect_args(lang_german "${OLD}|--lang=de_DE|${NATIVITY}" 0 "Pascha: 2026-04-12.+## Januar.+\\| Datum \\| Wochentag \\| Daten \\|.+2026-01-07 \\| Mittwoch")
  bcal_expect_args(lang_fallback "${OLD}|--lang=de_AT|${NATIVITY}" 0 "## J.+nner.+Mittwoch")   # months of de_AT, the rest of de_DE
  bcal_expect_args(lang_any_name "--year=2026|--old|-C|${CFG}/custom.yaml|--lang=foo_BAR|${NATIVITY}" 0 "Paschax:.+## Janx.+Wex")
  bcal_expect_args(lang_from_config "--year=2026|--old|-C|${CFG}/german.yaml|${NATIVITY}" 0 "## Januar.+Mittwoch")
  bcal_expect_args(lang_option_beats_config "--year=2026|--old|-C|${CFG}/german.yaml|--lang=en_US|${NATIVITY}" 0 "## January.+Wednesday")
  bcal_expect_args(lang_without_year "--lang=de_DE|${NATIVITY}" 3 "need --year")
  bcal_expect_args(lang_unknown "${OLD}|--lang=fr_FR|${NATIVITY}" 2 "cannot find the language file fr_FR.yaml in the lang_dir of ")
  bcal_expect_args(lang_not_set "--year=2026|--old|-C|${CFG}/no-language.yaml|${NATIVITY}" 1 "no language: set language: in the configuration or use --lang")
  bcal_expect_args(lang_dirs_missing "--year=2026|--old|-C|${CFG}/no-lang.yaml|${NATIVITY}" 2 "cannot find the language file en_US.yaml.+it lists no directory")
  # en_US.yaml falls back to de_XX which falls back to en_US: read only once, with a warning
  bcal_expect_args(lang_circular_fallback "--year=2026|--old|-C|${CFG}/circular.yaml|${NATIVITY}" 0 "## January.+already read")
  bcal_expect_args(lang_file_invalid "--year=2026|--old|-C|${CFG}/bad-lang.yaml|${NATIVITY}" 1 "en_US.yaml:1: months must be a list of 12 names")
  # Without -C and bcal.yaml the program uses the installed config.yaml.
  add_test(NAME default_config
           COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:byzantine-cal> -DCONFIG=${BCAL_CONFIG_FILE}
                   -DYAML=${NATIVITY} -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/default_config.cmake)
  # The configuration file: bcal.yaml in the working directory, -C wins over it.
  # (tests/config/cwd has its own template, so an installed config would give other output)
  bcal_expect_args(config_in_working_dir "--year=2026|${NATIVITY}" 0 "cwd template 2026" ${CFG}/cwd)
  bcal_expect_args(config_option_wins "--year=2026|-C|${CFG}/no-template.yaml|${NATIVITY}" 2
                   "cannot find the template markdown.inja in the template_dir of ${CFG}/no-template.yaml" ${CFG}/cwd)
  bcal_expect_args(config_missing "--year=2026|-C|${CFG}/nope.yaml|${NATIVITY}" 2 "cannot read configuration file")
  bcal_expect_args(config_invalid "--year=2026|-C|${CFG}/invalid.yaml|${NATIVITY}" 1 "invalid.yaml:1: template_dir must be a list")
  bcal_expect_args(config_without_year "-C|${CFG}/bcal.yaml|${NATIVITY}" 3 "need --year")
  add_test(NAME output_file
           COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:byzantine-cal>
                   "-DARGS=--year=2026|-C|${CFG}/bcal.yaml|--output=${CMAKE_CURRENT_BINARY_DIR}/out.md|${NATIVITY}"
                   -DCODE=0 "-DOUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/out.md"
                   "-DOUT_REGEX=# 2026.+Nativity" -P ${RUNNER})
  if(EXISTS /dev/full)   # every write fails: must be reported, also when only flushing
    bcal_expect_args(output_file_write_fails "--year=2026|-C|${CFG}/bcal.yaml|--output=/dev/full|${NATIVITY}" 2 "cannot write /dev/full")
  endif()
  bcal_expect_args(output_file_unwritable "--year=2026|-C|${CFG}/bcal.yaml|--output=${CMAKE_CURRENT_BINARY_DIR}/no/such/dir/out.md|${NATIVITY}" 2 "cannot write")
  bcal_expect_args(year_not_a_number "--year=abc|${NATIVITY}" 3 "whole number")
  bcal_expect_args(year_out_of_range "--year=0|${NATIVITY}" 3 "1..9999")
  bcal_expect_args(old_without_year "--old|${NATIVITY}" 3 "need --year")
  bcal_expect_args(year_invalid_file "--year=2026|${CMAKE_CURRENT_SOURCE_DIR}/tests/invalid/bad_day.yml" 1 "does not exist")
  add_test(NAME error_usage COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:byzantine-cal>
           -DCODE=3 -P ${RUNNER})
endif()
