cmake_minimum_required(VERSION 3.20)
project(MemoryManagerSimulator LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
option(MEMSIM_BUILD_VISUAL "Build the Qt 6 Widgets interface" OFF)

add_library(memsim-core STATIC core/memory_manager.cpp script/command.cpp)
target_compile_features(memsim-core PUBLIC cxx_std_17)
target_include_directories(memsim-core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(memsim-visual-session STATIC visual/session.cpp)
target_link_libraries(memsim-visual-session PUBLIC memsim-core)

add_executable(memsim cli/main.cpp cli/format.cpp)
target_link_libraries(memsim PRIVATE memsim-core)

if(MINGW)
    get_filename_component(_compiler_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
    foreach(runtime_dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
        if(NOT EXISTS "${_compiler_bin}/${runtime_dll}")
            message(FATAL_ERROR "Runtime MinGW ausente: ${_compiler_bin}/${runtime_dll}")
        endif()
        add_custom_command(TARGET memsim POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${_compiler_bin}/${runtime_dll}" "$<TARGET_FILE_DIR:memsim>"
            COMMENT "Copiando ${runtime_dll} do compilador usado")
    endforeach()
endif()

include(CTest)
if(BUILD_TESTING)
    add_test(NAME cli_smoke COMMAND "${CMAKE_COMMAND}"
        "-DMEMSIM=$<TARGET_FILE:memsim>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_smoke.cmake")
    set_tests_properties(cli_smoke PROPERTIES TIMEOUT 5)
    add_executable(core_tests tests/core_test.cpp)
    target_link_libraries(core_tests PRIVATE memsim-core)
    add_test(NAME core_tests COMMAND core_tests)
    add_executable(core_reference_tests tests/core_reference_test.cpp)
    target_link_libraries(core_reference_tests PRIVATE memsim-core)
    add_test(NAME core_reference_tests COMMAND core_reference_tests)

    add_executable(script_tests tests/script_test.cpp)
    target_link_libraries(script_tests PRIVATE memsim-core)
    add_test(NAME script_tests COMMAND script_tests)

    add_executable(cli_format_tests tests/cli_format_test.cpp cli/format.cpp)
    target_link_libraries(cli_format_tests PRIVATE memsim-core)
    add_test(NAME cli_format_tests COMMAND cli_format_tests)
    add_executable(visual_session_tests tests/visual_session_test.cpp)
    target_link_libraries(visual_session_tests PRIVATE memsim-visual-session)
    add_test(NAME visual_session_tests COMMAND visual_session_tests)
    add_executable(visual_examples_tests tests/visual_examples_test.cpp)
    target_link_libraries(visual_examples_tests PRIVATE memsim-visual-session)
    add_test(NAME visual_examples_tests COMMAND visual_examples_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/next-fit.cmd"
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/defrag.cmd"
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/divergencia.cmd"
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/falha-fragmentacao.cmd")
    function(add_visual_cli_check example occupied free_bytes)
        add_test(NAME cli_example_${example} COMMAND "${CMAKE_COMMAND}"
            "-DMEMSIM=$<TARGET_FILE:memsim>"
            "-DSCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/examples/${example}.cmd"
            "-DOCCUPIED=${occupied}" "-DFREE=${free_bytes}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/visual_cli_example.cmake")
    endfunction()
    add_visual_cli_check(next-fit 26 6)
    add_visual_cli_check(defrag 20 4)
    add_visual_cli_check(divergencia 17 47)
    add_visual_cli_check(falha-fragmentacao 8 16)
endif()

if(MEMSIM_BUILD_VISUAL)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)
    add_executable(memsim-visual visual/qt/main.cpp visual/qt/window.cpp
        visual/qt/memory_map.cpp visual/qt/step_panel.cpp visual/qt/playback_interval.cpp)
    target_link_libraries(memsim-visual PRIVATE memsim-visual-session Qt6::Widgets)

    if(WIN32)
        get_filename_component(_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
        find_program(MEMSIM_WINDEPLOYQT NAMES windeployqt HINTS "${_qt_prefix}/bin" REQUIRED)
        get_filename_component(_compiler_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
        get_filename_component(_qt_bin "${MEMSIM_WINDEPLOYQT}" DIRECTORY)
        add_custom_command(TARGET memsim-visual POST_BUILD
            COMMAND "${CMAKE_COMMAND}"
                "-DAPP=$<TARGET_FILE:memsim-visual>"
                "-DWINDEPLOYQT=${MEMSIM_WINDEPLOYQT}"
                "-DCOMPILER_BIN=${_compiler_bin}"
                "-DQT_BIN=${_qt_bin}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deploy_visual.cmake"
            COMMENT "Copiando DLLs e plugins Qt para o executável visual")
    endif()

    if(BUILD_TESTING)
        add_executable(visual_map_tests tests/visual_map_test.cpp visual/qt/memory_map.cpp)
        target_link_libraries(visual_map_tests PRIVATE memsim-core Qt6::Widgets)
        add_test(NAME visual_map_tests COMMAND visual_map_tests)
        set_tests_properties(visual_map_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 5)
        add_executable(visual_panel_tests tests/visual_panel_test.cpp visual/qt/step_panel.cpp)
        target_link_libraries(visual_panel_tests PRIVATE memsim-visual-session Qt6::Widgets)
        add_test(NAME visual_panel_tests COMMAND visual_panel_tests
            "${CMAKE_CURRENT_SOURCE_DIR}/examples/falha-fragmentacao.cmd")
        set_tests_properties(visual_panel_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 5)
        add_executable(visual_interval_tests tests/visual_interval_test.cpp visual/qt/playback_interval.cpp)
        target_link_libraries(visual_interval_tests PRIVATE Qt6::Core)
        target_include_directories(visual_interval_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
        add_test(NAME visual_interval_tests COMMAND visual_interval_tests)
        add_test(NAME visual_smoke COMMAND memsim-visual --smoke-test
                 "${CMAKE_CURRENT_SOURCE_DIR}/tests/prototipo-f01.cmd")
        set_tests_properties(visual_smoke PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            TIMEOUT 5
        )
        add_test(NAME visual_playback COMMAND memsim-visual --playback-test
                 "${CMAKE_CURRENT_SOURCE_DIR}/tests/prototipo-f01.cmd")
        set_tests_properties(visual_playback PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 5)
        foreach(example next-fit defrag divergencia falha-fragmentacao)
            add_test(NAME visual_${example} COMMAND memsim-visual --smoke-test
                "${CMAKE_CURRENT_SOURCE_DIR}/examples/${example}.cmd")
            set_tests_properties(visual_${example} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 5)
        endforeach()
    endif()
endif()
