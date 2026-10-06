# SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(deepinglassdecoration MODULE
    decoration.cpp
    button.cpp
)
target_include_directories(deepinglassdecoration PRIVATE ${CMAKE_SOURCE_DIR}/kde/common)
set_target_properties(deepinglassdecoration PROPERTIES OUTPUT_NAME org.plasmadeepin.glass)
target_link_libraries(deepinglassdecoration
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::DBus
        KF6::CoreAddons
        KF6::ConfigCore
        KDecoration3::KDecoration
)
install(TARGETS deepinglassdecoration DESTINATION ${KDE_INSTALL_PLUGINDIR}/${KDECORATION_PLUGIN_DIR})
