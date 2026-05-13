# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(daplink)

include(${ZEPHYR_BASE}/samples/subsys/usb/common/common.cmake)

FILE(GLOB app_sources src/*.c)

# Remove BULK v2 class for low-RAM boards
if(CONFIG_SOC_STM32F103X8)
  list(REMOVE_ITEM app_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/usbd_dap_v2.c)
  list(REMOVE_ITEM app_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/msc_lun.c)
  list(REMOVE_ITEM app_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/msc_daplink.c)
  list(REMOVE_ITEM app_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/flash_blob_stm32f103.c)
  list(REMOVE_ITEM app_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/flash_prog.c)
  list(REMOVE_ITEM app_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/intelhex.c)
endif()

target_sources(app PRIVATE ${app_sources})
