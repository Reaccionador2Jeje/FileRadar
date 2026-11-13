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
    set(FILERADAR_WX_LIBRARIES "")
    if(WIN32)
        # vcpkg provides wxWidgets as imported CMake targets.
        find_package(wxWidgets CONFIG QUIET COMPONENTS core base)
        if(TARGET wx::core AND TARGET wx::base)
            set(FILERADAR_WX_LIBRARIES wx::core wx::base)
        elseif(TARGET wxWidgets::wxWidgets)
            set(FILERADAR_WX_LIBRARIES wxWidgets::wxWidgets)
        endif()
    endif()

    if(NOT FILERADAR_WX_LIBRARIES)
        find_package(wxWidgets QUIET COMPONENTS core base)
        if(wxWidgets_FOUND)
            include(${wxWidgets_USE_FILE})
            set(FILERADAR_WX_LIBRARIES ${wxWidgets_LIBRARIES})
        endif()
    endif()

    if(FILERADAR_WX_LIBRARIES)
        if(WIN32)
            add_executable(fileradar-gui WIN32 src/gui_main.cpp src/gui.cpp)
        else()
            add_executable(fileradar-gui src/gui_main.cpp src/gui.cpp)
        endif()
        target_include_directories(fileradar-gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_link_libraries(fileradar-gui PRIVATE fileradar_core ${FILERADAR_WX_LIBRARIES})
        message(STATUS "FileRadar Desktop GUI enabled")
    else()
        message(STATUS "wxWidgets not found: building CLI/TUI only")
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
install(FILES README.md LICENSE DESTINATION share/doc/FileRadar)

# Native installer formats: NSIS on Windows, DEB on Linux, DMG on macOS.
set(CPACK_PACKAGE_NAME "FileRadar")
set(CPACK_PACKAGE_VENDOR "FileRadar Project")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cross-platform storage analyzer and duplicate finder")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Reaccionador2Jeje/FileRadar")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "FileRadar")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_FILE_NAME "FileRadar-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "FileRadar")
    set(CPACK_NSIS_PACKAGE_NAME "FileRadar")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
elseif(UNIX)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "FileRadar Project")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()

include(CPack)
