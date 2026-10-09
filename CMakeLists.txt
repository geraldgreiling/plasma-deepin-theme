# SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Builds the native parts of the Deepin theme for Plasma 6:
#   - Deepin Glass window decoration (KDecoration3 plugin)
#   - Deepin Glass application style (Qt 6 widget style)
#   - Deepin Glass Kirigami platform plugin (glass for Qt Quick applications)
# Optionally installs all data components (colours, Plasma style, global theme,
# icons, cursors, SDDM, Plymouth, GTK) system-wide with -DINSTALL_THEME_DATA=ON.

cmake_minimum_required(VERSION 3.22)
project(plasma-deepin-theme VERSION 1.0.0 LANGUAGES CXX)

set(QT_MIN_VERSION "6.6.0")
set(KF6_MIN_VERSION "6.0.0")

option(BUILD_DECORATION "Build the Deepin Glass window decoration" ON)
option(BUILD_STYLE "Build the Deepin Glass Qt application style" ON)
option(BUILD_KIRIGAMI "Build the Deepin Glass Kirigami platform plugin (glass for Qt Quick applications)" ON)
option(INSTALL_THEME_DATA "Install colour schemes, Plasma style, global theme, SDDM, Plymouth and GTK themes" OFF)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui Widgets DBus)
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS Config CoreAddons WindowSystem)

# default ~/.config/deepinglassrc, embedded into the plugins
file(READ ${CMAKE_SOURCE_DIR}/kde/common/deepinglassrc.default DEEPINGLASS_DEFAULT_CONFIG)
configure_file(kde/common/deepinglass_defaultconfig.h.in ${CMAKE_BINARY_DIR}/kde/common/deepinglass_defaultconfig.h @ONLY)
include_directories(${CMAKE_BINARY_DIR}/kde/common)

if(BUILD_DECORATION)
    find_package(KDecoration3 REQUIRED)
    add_subdirectory(kde/decoration)
endif()

if(BUILD_STYLE)
    find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Quick QuickWidgets)
    add_subdirectory(kde/style)
endif()

if(BUILD_KIRIGAMI)
    find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Qml Quick)
    find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS KirigamiPlatform ColorScheme IconThemes)
    add_subdirectory(kde/kirigami)
endif()

if(INSTALL_THEME_DATA)
    install(DIRECTORY color-schemes/ DESTINATION ${KDE_INSTALL_DATADIR}/color-schemes FILES_MATCHING PATTERN "*.colors")
    install(DIRECTORY plasma/desktoptheme/ DESTINATION ${KDE_INSTALL_DATADIR}/plasma/desktoptheme)
    install(DIRECTORY plasma/look-and-feel/ DESTINATION ${KDE_INSTALL_DATADIR}/plasma/look-and-feel)
    install(DIRECTORY sddm/ DESTINATION ${KDE_INSTALL_DATADIR}/sddm/themes)
    install(DIRECTORY plymouth/ DESTINATION ${KDE_INSTALL_DATADIR}/plymouth/themes)
    install(DIRECTORY gtk/Deepin-Glass gtk/Deepin-Glass-Dark DESTINATION ${KDE_INSTALL_DATADIR}/themes)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
