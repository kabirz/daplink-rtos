#include "DAP_config.h"
#include "DAP.h"

#if (DAP_JTAG != 0)

#define JTAG_CLOCK_CYCLE()              \
  PIN_SWCLK_TCK_CLR();                  \
  PIN_DELAY_SLOW(DAP_Data.clock_delay_jtag); \
  PIN_SWCLK_TCK_SET();                  \
  PIN_DELAY_SLOW(DAP_Data.clock_delay_jtag)

#define JTAG_TMS_WRITE(bit)             \
  PIN_SWDIO_TMS_SET();                  \
  PIN_SWDIO_OUT(bit);                   \
  JTAG_CLOCK_CYCLE()

#define JTAG_TDO_CAPTURE(bit)           \
  PIN_SWCLK_TCK_CLR();                  \
  PIN_DELAY_SLOW(DAP_Data.clock_delay_jtag); \
  bit = PIN_TDO_IN();                   \
  PIN_SWCLK_TCK_SET();                  \
  PIN_DELAY_SLOW(DAP_Data.clock_delay_jtag)

#define JTAG_TDI_TDO_WRITE(tdi, tdo)    \
  PIN_TDI_OUT(tdi);                     \
  PIN_SWCLK_TCK_CLR();                  \
  PIN_DELAY_SLOW(DAP_Data.clock_delay_jtag); \
  tdo = PIN_TDO_IN();                   \
  PIN_SWCLK_TCK_SET();                  \
  PIN_DELAY_SLOW(DAP_Data.clock_delay_jtag)

void JTAG_Sequence(uint32_t sequence_info, const uint8_t *request, uint8_t *response)
{
    uint32_t count;
    uint32_t tms;
    uint32_t val;
    uint32_t n;

    count = sequence_info & 0xFFU;
    if (count == 0U) {
        count = 64U;
    }

    tms = (sequence_info >> 8) & 0x01U;

    if ((sequence_info & JTAG_SEQUENCE_TDO) != 0U) {
        for (uint32_t i = 0; i < count; i++) {
            val = (i & 7U) ? (val >> 1) : *request++;
            PIN_SWDIO_TMS_SET();
            PIN_SWDIO_OUT(tms);
            PIN_TDI_OUT(val & 1U);
            JTAG_CLOCK_CYCLE();

            n = 7U - (i & 7U);
            if (n == 7U) {
                *response = 0U;
            }
            if (PIN_TDO_IN()) {
                *response |= (1U << n);
            }
            if (n == 0U) {
                response++;
            }
        }
    } else {
        val = 0U;
        n = 0U;
        for (uint32_t i = 0; i < count; i++) {
            if (n == 0U) {
                val = *request++;
                n = 8U;
            }
            PIN_SWDIO_TMS_SET();
            PIN_SWDIO_OUT(tms);
            PIN_TDI_OUT(val & 1U);
            JTAG_CLOCK_CYCLE();
            val >>= 1;
            n--;
        }
    }
}

uint8_t JTAG_Transfer(uint32_t request, uint32_t *data)
{
    (void)request;
    (void)data;
    return DAP_TRANSFER_OK;
}

uint8_t JTAG_WriteIR(uint32_t request, uint32_t *data)
{
    (void)request;
    (void)data;
    return DAP_TRANSFER_OK;
}

#endif /* DAP_JTAG */
