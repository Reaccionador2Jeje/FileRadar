cmake_minimum_required(VERSION 3.16)
project(FileRadar VERSION 1.3.0 LANGUAGES CXX)

option(FILERADAR_BUILD_TESTS "Build FileRadar tests" ON)
option(FILERADAR_BUILD_GUI "Build the wxWidgets desktop application when wxWidgets is available" ON)
option(FILERADAR_REQUIRE_GUI "Fail configuration if the desktop GUI cannot be built" OFF)

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
        if(APPLE)
            add_executable(fileradar-gui MACOSX_BUNDLE src/gui_main.cpp src/gui.cpp)
            set_target_properties(fileradar-gui PROPERTIES
                MACOSX_BUNDLE_BUNDLE_NAME "FileRadar"
                MACOSX_BUNDLE_GUI_IDENTIFIER "dev.atdev.fileradar"
                MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
                MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
            )
        elseif(WIN32)
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

if(FILERADAR_REQUIRE_GUI AND NOT TARGET fileradar-gui)
    message(FATAL_ERROR "FileRadar desktop GUI is required for this package, but wxWidgets was not found.")
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
    if(APPLE)
        install(TARGETS fileradar-gui BUNDLE DESTINATION .)
        install(CODE [[
            include(BundleUtilities)
            set(bundle "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/FileRadar.app")
            if(EXISTS "${bundle}")
                fixup_bundle("${bundle}" "" "")
            endif()
        ]])
    else()
        install(TARGETS fileradar-gui RUNTIME DESTINATION bin)
    endif()

    if(UNIX AND NOT APPLE)
        install(FILES packaging/fileradar.desktop DESTINATION share/applications)
        install(FILES packaging/fileradar.svg
            DESTINATION share/icons/hicolor/scalable/apps)
    endif()
endif()
install(FILES README.md LICENSE DESTINATION share/doc/FileRadar)

set(CPACK_PACKAGE_NAME "FileRadar")
set(CPACK_PACKAGE_VENDOR "AtDev")
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
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$DESKTOP\\FileRadar.lnk' '$INSTDIR\\bin\\fileradar-gui.exe'"
        "CreateShortCut '$SMPROGRAMS\\$STARTMENU_FOLDER\\FileRadar.lnk' '$INSTDIR\\bin\\fileradar-gui.exe'"
    )
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
elseif(UNIX)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "AtDev")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()

include(CPack)
