#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

# Add custom devicetree bindings directory
list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

# USB device stack helpers (local copy)
set(SAMPLE_USBD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/usb)
target_include_directories(app PRIVATE ${SAMPLE_USBD_DIR})
target_sources_ifdef(CONFIG_USB_DEVICE_STACK_NEXT app PRIVATE
  ${SAMPLE_USBD_DIR}/sample_usbd_init.c
)

target_sources(app PRIVATE
  src/json_parser.c
  src/main.c
  src/file_manager.c
)
