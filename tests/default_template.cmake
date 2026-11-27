# Usage: cmake -DEXE=<exe> -DTPL_DIR=<compiled-in template dir> -DYAML=<file> -P default_template.cmake
# The program reads TPL_DIR/markdown.inja: if it is installed
# there the Markdown is rendered, otherwise the program must name the missing file.
execute_process(COMMAND ${EXE} --year=2026 ${YAML}
                RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(EXISTS ${TPL_DIR}/markdown.inja)
  if(NOT rc EQUAL 0 OR NOT out MATCHES "# 2026.+Pascha: 2026-04-12")
    message(FATAL_ERROR "installed template not used (exit ${rc}):\n${out}${err}")
  endif()
else()
  if(NOT rc EQUAL 2 OR NOT err MATCHES "cannot read template[ \n]+${TPL_DIR}/markdown.inja")
    message(FATAL_ERROR "expected exit 2 naming ${TPL_DIR}/markdown.inja, got ${rc}:\n${out}${err}")
  endif()
endif()
