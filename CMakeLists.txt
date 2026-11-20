cmake_minimum_required(VERSION 3.20)
project(MemoryManagerSimulator LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
option(MEMSIM_BUILD_VISUAL "Build the Qt Widgets prototype" OFF)

add_library(memsim-core STATIC core/memory_manager.cpp)
target_compile_features(memsim-core PUBLIC cxx_std_17)
target_include_directories(memsim-core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(memsim cli/main.cpp)
target_link_libraries(memsim PRIVATE memsim-core)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME cli_smoke COMMAND memsim)
    set_tests_properties(cli_smoke PROPERTIES
        PASS_REGULAR_EXPRESSION "Simulador de gerenciamento de memoria"
    )
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
