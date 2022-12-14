# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
set(QEMU_EXTRA_FLAGS -s)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(console)

target_sources(app PRIVATE 
    src/main.c
    src/provision.c
    src/uart.c
    src/board.c
    src/heartbeat.c
    src/msgdata.c
    src/microphone.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/includes)
