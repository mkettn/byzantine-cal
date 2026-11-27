# Usage: cmake -DEXE=<exe> -DARGS=<a|b> -DCODE=<exit code> [-DREGEX=<regex>] -P run_expect.cmake
# Arguments are separated by "|".
if(DEFINED ARGS)
  string(REPLACE "|" ";" ARGS "${ARGS}")
endif()
execute_process(COMMAND ${EXE} ${ARGS}
                RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL CODE)
  message(FATAL_ERROR "expected exit ${CODE}, got ${rc}\n${out}${err}")
endif()
if(DEFINED REGEX AND NOT "${out}${err}" MATCHES "${REGEX}")
  message(FATAL_ERROR "output does not match '${REGEX}':\n${out}${err}")
endif()
