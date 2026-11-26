execute_process(COMMAND "${MEMSIM}" INPUT_FILE "${SCRIPT}"
    OUTPUT_VARIABLE output ERROR_VARIABLE diagnostic RESULT_VARIABLE status ENCODING UTF-8)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "CLI falhou: ${diagnostic}")
endif()
if(NOT output MATCHES "Ocupado: ${OCCUPIED} bytes \\| Livre: ${FREE} bytes")
    message(FATAL_ERROR "Estatisticas divergentes: ${output}")
endif()
