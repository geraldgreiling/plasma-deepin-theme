# SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.22)
project(qmlshot LANGUAGES CXX)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Quick)
add_executable(qmlshot main.cpp)
target_link_libraries(qmlshot Qt6::Quick)
