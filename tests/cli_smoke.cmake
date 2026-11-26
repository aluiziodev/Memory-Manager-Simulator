execute_process(
    COMMAND "${MEMSIM}"
    INPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/cli-session.cmd"
    OUTPUT_VARIABLE output
    ERROR_VARIABLE diagnostics
    RESULT_VARIABLE status
    ENCODING UTF-8
)

set(stats [=[== Estatísticas ==
Tamanho total: 8 bytes
Ocupado: 5 bytes | Livre: 3 bytes
Buracos (fragmentação externa): 1
Maior buraco livre: 3 bytes
Uso efetivo: 62.50%
]=])

set(expected [=[Erro: linha 5: Comando desconhecido.
Erro: sem espaço contíguo para 6 bytes (livre total: 5, maior buraco: 5)
Mapa de Memória (8 bytes)
----------
[#####...]
[11122...]
----------

Blocos ativos: [id=1] @0 +3B (usado=3B) | [id=2] @3 +2B (usado=2B)
]=])
string(APPEND expected "${stats}${stats}")

if(NOT status STREQUAL "0" OR NOT diagnostics STREQUAL "" OR NOT output STREQUAL expected)
    message(FATAL_ERROR "Status: ${status}\nDiagnóstico: ${diagnostics}\nEsperado:\n${expected}\nObtido:\n${output}")
endif()
