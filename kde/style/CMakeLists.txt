# SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(deepinglassstyle MODULE
    deepinglassstyle.cpp
    deepinglassplugin.cpp
)
target_include_directories(deepinglassstyle PRIVATE ${CMAKE_SOURCE_DIR}/kde/common)
set_target_properties(deepinglassstyle PROPERTIES OUTPUT_NAME deepinglass6)
target_link_libraries(deepinglassstyle
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Quick
        Qt6::QuickWidgets
        KF6::ConfigCore
        KF6::WindowSystem
)
install(TARGETS deepinglassstyle DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/styles)
install(FILES deepinglass.themerc DESTINATION ${KDE_INSTALL_DATADIR}/kstyle/themes)
