cmake_minimum_required(VERSION 3.16)
project(RetroClock VERSION 2.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(RetroClock WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.h mainwindow.cpp
    clockface.h clockface.cpp
    autostart.h autostart.cpp
)

# 各平台程序图标
if(WIN32)
    target_sources(RetroClock PRIVATE resources/retroclock.rc)
elseif(APPLE)
    set(MACOS_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/retroclock.icns)
    set_source_files_properties(${MACOS_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(RetroClock PRIVATE ${MACOS_ICON})
endif()

target_compile_definitions(RetroClock PRIVATE APP_VERSION="${PROJECT_VERSION}")
target_link_libraries(RetroClock PRIVATE Qt6::Widgets)

set_target_properties(RetroClock PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "RetroClock"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.retroclock.app"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_ICON_FILE retroclock.icns
)

include(GNUInstallDirs)
install(TARGETS RetroClock
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
    install(FILES resources/retroclock.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES resources/retroclock.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps)
endif()
