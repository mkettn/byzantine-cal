# Usage: cmake -DEXE=<exe> -DCONFIG=<installed config file> -DYAML=<file> -P default_config.cmake
# Without -C and without bcal.yaml in the working directory the program uses the
# installed config file: if there is one the Markdown is rendered, otherwise the
# program must say that no configuration file was found.
execute_process(COMMAND ${EXE} --year=2026 ${YAML}
                RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(EXISTS ${CONFIG})
  if(NOT rc EQUAL 0 OR NOT out MATCHES "# 2026.+Pascha: 2026-04-12")
    message(FATAL_ERROR "installed config not used (exit ${rc}):\n${out}${err}")
  endif()
else()
  if(NOT rc EQUAL 2 OR NOT err MATCHES "no configuration file found")
    message(FATAL_ERROR "expected exit 2 'no configuration file found', got ${rc}:\n${out}${err}")
  endif()
endif()
