# Executado depois do link no Windows para tornar o executável Qt independente do PATH.
set(ENV{PATH} "${COMPILER_BIN};${QT_BIN};$ENV{PATH}")
execute_process(
    COMMAND "${WINDEPLOYQT}" --release --compiler-runtime --no-translations
        --include-plugins qoffscreen "${APP}"
    RESULT_VARIABLE status
)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "windeployqt falhou (${status}) para ${APP}")
endif()
