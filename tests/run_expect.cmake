# Usage: cmake -DEXE=<exe> -DARGS=<a|b> -DCODE=<exit code> [-DREGEX=<regex>] -P run_expect.cmake
# Arguments are separated by "|". With -DOUT_FILE=<file> -DOUT_REGEX=<regex> the
# file written by the program is checked as well (it is removed first).
if(DEFINED OUT_FILE)
  file(REMOVE ${OUT_FILE})
endif()

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
if(DEFINED OUT_FILE)
  file(READ ${OUT_FILE} written)
  if(NOT written MATCHES "${OUT_REGEX}")
    message(FATAL_ERROR "${OUT_FILE} does not match '${OUT_REGEX}':\n${written}")
  endif()
endif()
