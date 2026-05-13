/**
 * @file    IO_Config.h
 * @brief   CMSIS-DAP GPIO Pin Configuration for Zephyr
 *
 * DAPLink Interface Firmware
 * Copyright (c) 2009-2016, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Ported to Zephyr RTOS
 */

#ifndef __IO_CONFIG_H__
#define __IO_CONFIG_H__

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * SWJ Debug Port Pin Mapping
 *
 * These must be configured for your specific board.
 * Either define them here or via devicetree.
 *============================================================================*/

/* Default pin configuration - override for your board */
/* Format: PORT, PIN */
#ifndef SWCLK_TCK_SPEC
#define SWCLK_TCK_SPEC  GPIO_DT_SPEC_GET(DT_ALIAS(swclk), gpios)
#endif

#ifndef SWDIO_TMS_SPEC
#define SWDIO_TMS_SPEC  GPIO_DT_SPEC_GET(DT_ALIAS(swdio), gpios)
#endif

#ifndef NRST_SPEC
#define NRST_SPEC       GPIO_DT_SPEC_GET(DT_ALIAS(nrst), gpios)
#endif

/* Initialize debug port pins */
void swj_gpio_init(void);

/* SWCLK/TCK pin control */
void swclk_tck_set(void);
void swclk_tck_clr(void);
uint8_t swclk_tck_in(void);

/* SWDIO/TMS pin control */
void swdio_tms_set(void);
void swdio_tms_clr(void);
void swdio_tms_out(uint8_t bit);
uint8_t swdio_tms_in(void);
void swdio_out_enable(void);
void swdio_out_disable(void);

/* TDI pin control (JTAG) */
void tdi_out(uint8_t bit);
uint8_t tdi_in(void);

/* TDO pin control (JTAG) */
uint8_t tdo_in(void);

/* nTRST pin control (JTAG) */
void ntrst_out(uint8_t bit);
uint8_t ntrst_in(void);

/* nRESET pin control */
void nreset_out(uint8_t bit);
uint8_t nreset_in(void);

#ifdef __cplusplus
}
#endif

#endif /* __IO_CONFIG_H__ */
