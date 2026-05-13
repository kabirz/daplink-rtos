/**
 * @file    DAP_config.h
 * @brief   CMSIS-DAP Configuration File for Zephyr
 *
 * DAPLink Interface Firmware
 * Copyright (c) 2013-2021 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Ported to Zephyr RTOS
 */

#ifndef __DAP_CONFIG_H__
#define __DAP_CONFIG_H__

#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include "IO_Config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 *      Processor Clock
 *============================================================================*/
/** Specifies the CPU Clock in Hz for SWD/JTAG clock calculation.
 *  Uses Zephyr's SYS_CLOCK_HW_CYCLES_PER_SEC which is board-independent.
 */
#define CPU_CLOCK               CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC

/** Number of processor cycles for I/O Port write operations. */
#define IO_PORT_WRITE_CYCLES    2U

/*==============================================================================
 *      CMSIS-DAP Debug Unit Communication Packet Size
 *============================================================================*/
/** Maximum packet size for the debug unit. */
#ifndef DAP_PACKET_SIZE
#define DAP_PACKET_SIZE         64U
#endif

/** Maximum packet count for the debug unit. */
#ifndef DAP_PACKET_COUNT
#define DAP_PACKET_COUNT        4U
#endif

/*==============================================================================
 *      CMSIS-DAP Debug Unit Supported Ports
 *============================================================================*/
/** Support SWD mode. */
#ifndef DAP_SWD
#define DAP_SWD                 1
#endif

/** Support JTAG mode. */
#ifndef DAP_JTAG
#define DAP_JTAG                0
#endif

/** Number of JTAG devices in chain. */
#ifndef DAP_JTAG_DEV_CNT
#define DAP_JTAG_DEV_CNT        0
#endif

/** Support SWO UART mode. */
#ifndef SWO_UART
#define SWO_UART                0
#endif

/** Support SWO Manchester mode. */
#ifndef SWO_MANCHESTER
#define SWO_MANCHESTER          0
#endif

/** Activate timestamp clock. */
#ifndef TIMESTAMP_CLOCK
#define TIMESTAMP_CLOCK         0U
#endif

/*==============================================================================
 *      CMSIS-DAP Debug Unit Information
 *============================================================================*/
/** Vendor name. */
#define DAP_VENDOR_NAME          "ARM"

/** Product name. */
#define DAP_PRODUCT_NAME         "CMSIS-DAP v1/v2 (Zephyr)"

/** Serial number. */
#define DAP_SER_NUM              "123456"

/** Firmware version string. */
#ifdef DAP_FW_V1
#define DAP_FW_VER               "1.3.0"
#else
#define DAP_FW_VER               "2.1.0"
#endif

/** Target device vendor (optional). */
#define DAP_DEVICE_VENDOR        ""

/** Target device name (optional). */
#define DAP_DEVICE_NAME          ""

/** Board vendor (optional). */
#define DAP_BOARD_VENDOR         "Zephyr Project"

/** Board name (optional). */
#define DAP_BOARD_NAME           "DAPLink on Zephyr"

/*==============================================================================
 *      CMSIS-DAP Protocol Configuration
 *============================================================================*/
/** Number of SWD transfers that can be batched. */
#define DAP_TRANSFER_BUFFER_SIZE 1024U

/** Maximum number of SWD/JTAG retries. */
#define DAP_TRANSFER_RETRIES     100U

/** Default SWD/JTAG clock frequency in Hz. */
#define DAP_DEFAULT_SWJ_CLOCK    1000000U

/*==============================================================================
 *      Pin IO Macros (mapped to IO_Config.h functions)
 *============================================================================*/

/* SWCLK / TCK */
#define PIN_SWCLK_TCK_SET()             swclk_tck_set()
#define PIN_SWCLK_TCK_CLR()             swclk_tck_clr()
#define PIN_SWCLK_TCK_IN()              swclk_tck_in()

/* SWDIO / TMS */
#define PIN_SWDIO_TMS_SET()             swdio_tms_set()
#define PIN_SWDIO_TMS_CLR()             swdio_tms_clr()
#define PIN_SWDIO_TMS_IN()              swdio_tms_in()
#define PIN_SWDIO_OUT(bit)              swdio_tms_out(bit)
#define PIN_SWDIO_IN()                  swdio_tms_in()
#define PIN_SWDIO_OUT_ENABLE()          swdio_out_enable()
#define PIN_SWDIO_OUT_DISABLE()         swdio_out_disable()

/* TDI */
#define PIN_TDI_OUT(bit)                tdi_out(bit)
#define PIN_TDI_IN()                    tdi_in()

/* TDO */
#define PIN_TDO_IN()                    tdo_in()

/* nTRST */
#define PIN_nTRST_OUT(bit)              ntrst_out(bit)
#define PIN_nTRST_IN()                  ntrst_in()

/* nRESET */
#define PIN_nRESET_OUT(bit)             nreset_out(bit)
#define PIN_nRESET_IN()                 nreset_in()

/*==============================================================================
 *      SWO Pin Macros
 *============================================================================*/
#if (SWO_UART != 0)
#define SWO_UART_IN()                   swo_uart_in()
#define SWO_UART_OUT(bit)               swo_uart_out(bit)
#endif

#if (SWO_MANCHESTER != 0)
#define SWO_MANCHESTER_IN()             swo_manchester_in()
#define SWO_MANCHESTER_OUT(bit)         swo_manchester_out(bit)
#endif

/*==============================================================================
 *      Timing and Delay Macros
 *============================================================================*/

/* Number of CPU cycles for a I/O port write */
#if defined(IO_PORT_WRITE_CYCLES)
#define IO_PORT_WRITE_CYCLES_CORRECTED  IO_PORT_WRITE_CYCLES
#else
#define IO_PORT_WRITE_CYCLES_CORRECTED  2U
#endif

/* Calculate delay for a given SWJ clock frequency */
#define DAP_CLOCK_DELAY(clock_hz)       ((CPU_CLOCK / clock_hz) / (2U * IO_PORT_WRITE_CYCLES_CORRECTED))

/**
 * \brief Slow delay for SWJ pin IO.
 * \param delay_cycles Number of clock cycles to delay.
 */
#if defined(__GNUC__)
static inline void PIN_DELAY_SLOW(uint32_t delay_cycles) {
    if (delay_cycles == 0U) return;
    uint32_t cycles = (delay_cycles * IO_PORT_WRITE_CYCLES_CORRECTED) / 2U;
    if (cycles > 0U) {
        __asm__ volatile (
            "1: subs %0, %0, #1 \n"
            "   bne 1b \n"
            : "+r" (cycles)
            :
            : "cc"
        );
    }
}
#else
static inline void PIN_DELAY_SLOW(uint32_t delay_cycles) {
    volatile uint32_t d = delay_cycles;
    while (d--);
}
#endif

/** Fast delay for SWJ pin IO (single cycle). */
#define PIN_DELAY_FAST()                ((void)0)

/*==============================================================================
 *      Timestamp
 *============================================================================*/
#if (TIMESTAMP_CLOCK != 0U)
extern uint32_t timestamp_clk_freq;
#define TIMESTAMP_CLK_FREQ              timestamp_clk_freq

/* Get timestamp value */
extern uint32_t TIMESTAMP_GET(void);
#else
#define TIMESTAMP_CLK_FREQ              0U
#define TIMESTAMP_GET()                 0U
#endif

/*==============================================================================
 *      UART for CDC ACM bridge
 *============================================================================*/
/** UART receive buffer size */
#ifndef DAP_UART_RX_BUF_SIZE
#define DAP_UART_RX_BUF_SIZE            1024U
#endif

/** UART transmit buffer size */
#ifndef DAP_UART_TX_BUF_SIZE
#define DAP_UART_TX_BUF_SIZE            1024U
#endif

#ifdef __cplusplus
}
#endif

#endif /* __DAP_CONFIG_H__ */
