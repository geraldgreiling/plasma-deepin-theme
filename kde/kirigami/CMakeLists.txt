# SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Kirigami platform plugin with frosted glass backgrounds.
#
# Kirigami loads the first plugin in kf6/kirigami/platform whose file name
# contains the name of the Qt Quick Controls style ("org.kde.desktop" in Plasma).
# Directory entries are sorted by name, so "org.kde.desktop.deepinglass" is found
# before qqc2-desktop-style's "org.kde.desktop" and replaces it. The plugin is a
# copy of qqc2-desktop-style's plugin plus translucent backgrounds; with
# [QtQuick] Translucent=false it behaves like the original.

add_library(deepinglasskirigami MODULE
    glassfactory.cpp
    glasscontroller.cpp
    glasstheme.cpp
    plasmadesktopunits.cpp
    animationspeedprovider.cpp
)
target_include_directories(deepinglasskirigami PRIVATE ${CMAKE_SOURCE_DIR}/kde/common)
set_target_properties(deepinglasskirigami PROPERTIES OUTPUT_NAME org.kde.desktop.deepinglass)
target_compile_definitions(deepinglasskirigami PRIVATE HAVE_QTDBUS=1)
target_link_libraries(deepinglasskirigami
    PRIVATE
        Qt6::Core
        Qt6::DBus
        Qt6::Qml
        Qt6::Quick
        KF6::KirigamiPlatform
        KF6::ConfigCore
        KF6::ColorScheme
        KF6::IconThemes
        KF6::WindowSystem
)
install(TARGETS deepinglasskirigami DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/kirigami/platform)
