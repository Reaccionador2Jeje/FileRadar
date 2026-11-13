cmake_minimum_required(VERSION 3.16)
project(FileRadar VERSION 1.3.0 LANGUAGES CXX)

option(FILERADAR_BUILD_TESTS "Build FileRadar tests" ON)
option(FILERADAR_BUILD_GUI "Build the wxWidgets desktop application when wxWidgets is available" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(fileradar_core
    src/scanner.cpp
    src/analyzer.cpp
    src/duplicates.cpp
    src/reports.cpp
    src/tui.cpp
)
target_include_directories(fileradar_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(fileradar_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(fileradar_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(fileradar src/main.cpp)
target_link_libraries(fileradar PRIVATE fileradar_core)

if(FILERADAR_BUILD_GUI)
    find_package(wxWidgets QUIET COMPONENTS core base)
    if(wxWidgets_FOUND)
        include(${wxWidgets_USE_FILE})
        if(WIN32)
            add_executable(fileradar-gui WIN32 src/gui_main.cpp src/gui.cpp)
        else()
            add_executable(fileradar-gui src/gui_main.cpp src/gui.cpp)
        endif()
        target_include_directories(fileradar-gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_link_libraries(fileradar-gui PRIVATE fileradar_core ${wxWidgets_LIBRARIES})
        message(STATUS "FileRadar Desktop GUI enabled (wxWidgets found)")
    else()
        message(STATUS "wxWidgets not found: building CLI/TUI only. Install wxWidgets to enable fileradar-gui.")
    endif()
endif()

if(FILERADAR_BUILD_TESTS)
    enable_testing()
    add_executable(fileradar_tests tests/test_core.cpp)
    target_link_libraries(fileradar_tests PRIVATE fileradar_core)
    add_test(NAME fileradar_core_tests COMMAND fileradar_tests)
    add_test(NAME fileradar_cli_help COMMAND fileradar --help)
endif()

install(TARGETS fileradar RUNTIME DESTINATION bin)
if(TARGET fileradar-gui)
    install(TARGETS fileradar-gui RUNTIME DESTINATION bin)
endif()
