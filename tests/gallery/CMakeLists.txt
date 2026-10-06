# SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.22)
project(deepinglass-gallery LANGUAGES CXX)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
set(CMAKE_AUTOMOC ON)
add_executable(gallery main.cpp)
target_link_libraries(gallery Qt6::Widgets)
