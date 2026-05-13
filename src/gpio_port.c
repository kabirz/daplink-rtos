#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/__assert.h>
#include "DAP_config.h"
#include "IO_Config.h"

/* SWJ debug port GPIO specs - override via devicetree aliases */
/* Expected aliases: swclk, swdio, nrst, tdi, tdo, ntrst */

static const struct gpio_dt_spec swclk_spec = GPIO_DT_SPEC_GET_OR(DT_ALIAS(swclk), gpios, {0});
static const struct gpio_dt_spec swdio_spec = GPIO_DT_SPEC_GET_OR(DT_ALIAS(swdio), gpios, {0});
static const struct gpio_dt_spec nrst_spec  = GPIO_DT_SPEC_GET_OR(DT_ALIAS(nrst), gpios, {0});

#if (DAP_JTAG != 0)
static const struct gpio_dt_spec tdi_spec   = GPIO_DT_SPEC_GET_OR(DT_ALIAS(tdi), gpios, {0});
static const struct gpio_dt_spec tdo_spec   = GPIO_DT_SPEC_GET_OR(DT_ALIAS(tdo), gpios, {0});
static const struct gpio_dt_spec ntrst_spec = GPIO_DT_SPEC_GET_OR(DT_ALIAS(ntrst), gpios, {0});
#endif

static bool gpio_inited = false;

void swj_gpio_init(void)
{
    if (gpio_inited) return;

    /* Initialize SWCLK */
    if (swclk_spec.port && device_is_ready(swclk_spec.port)) {
        gpio_pin_configure_dt(&swclk_spec, GPIO_OUTPUT_INIT_LOW);
    }

    /* Initialize SWDIO */
    if (swdio_spec.port && device_is_ready(swdio_spec.port)) {
        gpio_pin_configure_dt(&swdio_spec, GPIO_OUTPUT_INIT_LOW);
    }

    /* Initialize nRESET */
    if (nrst_spec.port && device_is_ready(nrst_spec.port)) {
        gpio_pin_configure_dt(&nrst_spec, GPIO_OUTPUT_INIT_HIGH);
    }

#if (DAP_JTAG != 0)
    if (tdi_spec.port && device_is_ready(tdi_spec.port)) {
        gpio_pin_configure_dt(&tdi_spec, GPIO_OUTPUT_INIT_LOW);
    }
    if (tdo_spec.port && device_is_ready(tdo_spec.port)) {
        gpio_pin_configure_dt(&tdo_spec, GPIO_INPUT);
    }
    if (ntrst_spec.port && device_is_ready(ntrst_spec.port)) {
        gpio_pin_configure_dt(&ntrst_spec, GPIO_OUTPUT_INIT_HIGH);
    }
#endif

    gpio_inited = true;
}

/* SWCLK / TCK */
void swclk_tck_set(void)
{
    if (swclk_spec.port) gpio_pin_set_dt(&swclk_spec, 1);
}

void swclk_tck_clr(void)
{
    if (swclk_spec.port) gpio_pin_set_dt(&swclk_spec, 0);
}

uint8_t swclk_tck_in(void)
{
    if (swclk_spec.port) return (uint8_t)gpio_pin_get_dt(&swclk_spec);
    return 0;
}

/* SWDIO / TMS */
void swdio_tms_set(void)
{
    if (swdio_spec.port) gpio_pin_set_dt(&swdio_spec, 1);
}

void swdio_tms_clr(void)
{
    if (swdio_spec.port) gpio_pin_set_dt(&swdio_spec, 0);
}

void swdio_tms_out(uint8_t bit)
{
    if (swdio_spec.port) gpio_pin_set_dt(&swdio_spec, bit);
}

uint8_t swdio_tms_in(void)
{
    if (swdio_spec.port) return (uint8_t)gpio_pin_get_dt(&swdio_spec);
    return 0;
}

void swdio_out_enable(void)
{
    if (swdio_spec.port) gpio_pin_configure_dt(&swdio_spec, GPIO_OUTPUT);
}

void swdio_out_disable(void)
{
    if (swdio_spec.port) gpio_pin_configure_dt(&swdio_spec, GPIO_INPUT);
}

/* TDI (JTAG) */
void tdi_out(uint8_t bit)
{
#if (DAP_JTAG != 0)
    if (tdi_spec.port) gpio_pin_set_dt(&tdi_spec, bit);
#else
    (void)bit;
#endif
}

uint8_t tdi_in(void)
{
#if (DAP_JTAG != 0)
    if (tdi_spec.port) return (uint8_t)gpio_pin_get_dt(&tdi_spec);
#endif
    return 0;
}

/* TDO (JTAG) */
uint8_t tdo_in(void)
{
#if (DAP_JTAG != 0)
    if (tdo_spec.port) return (uint8_t)gpio_pin_get_dt(&tdo_spec);
#endif
    return 0;
}

/* nTRST (JTAG) */
void ntrst_out(uint8_t bit)
{
#if (DAP_JTAG != 0)
    if (ntrst_spec.port) gpio_pin_set_dt(&ntrst_spec, bit);
#else
    (void)bit;
#endif
}

uint8_t ntrst_in(void)
{
#if (DAP_JTAG != 0)
    if (ntrst_spec.port) return (uint8_t)gpio_pin_get_dt(&ntrst_spec);
#endif
    return 0;
}

/* nRESET */
void nreset_out(uint8_t bit)
{
    if (nrst_spec.port) gpio_pin_set_dt(&nrst_spec, bit);
}

uint8_t nreset_in(void)
{
    if (nrst_spec.port) return (uint8_t)gpio_pin_get_dt(&nrst_spec);
    return 0;
}
