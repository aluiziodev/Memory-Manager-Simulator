cmake_minimum_required(VERSION 3.20)
project(MemoryManagerSimulator LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
option(MEMSIM_BUILD_VISUAL "Build the Qt Widgets prototype" OFF)

add_library(memsim-core STATIC core/memory_manager.cpp script/command.cpp)
target_compile_features(memsim-core PUBLIC cxx_std_17)
target_include_directories(memsim-core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(memsim cli/main.cpp cli/format.cpp)
target_link_libraries(memsim PRIVATE memsim-core)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME cli_smoke COMMAND memsim)
    set_tests_properties(cli_smoke PROPERTIES
        PASS_REGULAR_EXPRESSION "Simulador de gerenciamento de memoria"
    )
    add_executable(core_tests tests/core_test.cpp)
    target_link_libraries(core_tests PRIVATE memsim-core)
    add_test(NAME core_tests COMMAND core_tests)

    add_executable(script_tests tests/script_test.cpp)
    target_link_libraries(script_tests PRIVATE memsim-core)
    add_test(NAME script_tests COMMAND script_tests)

    add_executable(cli_format_tests tests/cli_format_test.cpp cli/format.cpp)
    target_link_libraries(cli_format_tests PRIVATE memsim-core)
    add_test(NAME cli_format_tests COMMAND cli_format_tests)
endif()

if(MEMSIM_BUILD_VISUAL)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)
    add_executable(memsim-visual visual/qt/main.cpp)
    target_link_libraries(memsim-visual PRIVATE memsim-core Qt6::Widgets)

    if(BUILD_TESTING)
        add_test(NAME visual_smoke COMMAND memsim-visual --smoke-test
                 "${CMAKE_CURRENT_SOURCE_DIR}/tests/prototipo-f01.cmd")
        set_tests_properties(visual_smoke PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            TIMEOUT 5
        )
    endif()
endif()
