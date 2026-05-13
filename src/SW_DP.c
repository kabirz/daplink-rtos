#include "DAP_config.h"
#include "DAP.h"

#if (DAP_SWD != 0)

#define SW_CLOCK_CYCLE()                \
  PIN_SWCLK_TCK_CLR();                  \
  PIN_DELAY_SLOW(DAP_Data.clock_delay); \
  PIN_SWCLK_TCK_SET();                  \
  PIN_DELAY_SLOW(DAP_Data.clock_delay)

#define SW_WRITE_BIT(bit)               \
  PIN_SWDIO_OUT(bit);                   \
  PIN_SWCLK_TCK_CLR();                  \
  PIN_DELAY_SLOW(DAP_Data.clock_delay); \
  PIN_SWCLK_TCK_SET();                  \
  PIN_DELAY_SLOW(DAP_Data.clock_delay)

#define SW_READ_BIT(bit)                \
  PIN_SWCLK_TCK_CLR();                  \
  PIN_DELAY_SLOW(DAP_Data.clock_delay); \
  bit = PIN_SWDIO_IN();                 \
  PIN_SWCLK_TCK_SET();                  \
  PIN_DELAY_SLOW(DAP_Data.clock_delay)

void SWJ_Sequence(uint32_t count, const uint8_t *data)
{
    uint32_t val;
    uint32_t n;

    val = 0U;
    n = 0U;
    while (count--) {
        if (n == 0U) {
            val = *data++;
            n = 8U;
        }
        if (val & 1U) {
            PIN_SWDIO_TMS_SET();
        } else {
            PIN_SWDIO_TMS_CLR();
        }
        SW_CLOCK_CYCLE();
        val >>= 1;
        n--;
    }
}

void SWD_Sequence(uint32_t info, const uint8_t *request, uint8_t *response)
{
    uint32_t count;
    uint32_t val;
    uint32_t n;

    count = info & 0xFFU;
    if (count == 0U) {
        count = 64U;
    }

    if ((info & SWD_SEQUENCE_DIN) != 0U) {
        while (count--) {
            SW_READ_BIT(val);
            n = 7U - (count & 7U);
            if (n == 7U) {
                *response = 0U;
            }
            if (val) {
                *response |= (1U << n);
            }
            if (n == 0U) {
                response++;
            }
        }
    } else {
        val = 0U;
        n = 0U;
        while (count--) {
            if (n == 0U) {
                val = *request++;
                n = 8U;
            }
            SW_WRITE_BIT(val & 1U);
            val >>= 1;
            n--;
        }
    }
}

uint8_t SWD_Transfer(uint32_t request, uint32_t *data)
{
    uint32_t val;
    uint32_t parity;
    uint32_t resp;
    uint32_t retry;
    uint32_t ack;

    if (request == 0U) {
        return DAP_TRANSFER_INVALID;
    }

    retry = DAP_Data.transfer.retry_count;
    ack = 0U;

    do {
        if (DAP_TransferAbort) {
            return DAP_TRANSFER_ERROR;
        }

        PIN_SWDIO_OUT_ENABLE();

        SW_WRITE_BIT(0U);
        SW_WRITE_BIT(0U);
        SW_WRITE_BIT(0U);
        SW_WRITE_BIT(0U);
        SW_WRITE_BIT(0U);
        SW_WRITE_BIT(0U);
        SW_WRITE_BIT(0U);
        SW_WRITE_BIT(0U);

        SW_WRITE_BIT(0x81U & 0x01U);

        val = request;
        parity = 0U;
        for (uint32_t i = 0; i < 32; i++) {
            SW_WRITE_BIT(val & 1U);
            parity ^= (val & 1U);
            val >>= 1;
        }
        SW_WRITE_BIT(parity);

        SW_CLOCK_CYCLE();

        PIN_SWDIO_OUT_DISABLE();

        SW_CLOCK_CYCLE();

        SW_READ_BIT(resp);
        resp &= 0x01U;
        SW_READ_BIT(val);
        resp |= (val << 1);
        SW_READ_BIT(val);
        resp |= (val << 2);

        if (resp == 0x01U) {
            ack = DAP_TRANSFER_WAIT;
        } else if (resp == 0x02U) {
            ack = DAP_TRANSFER_FAULT;
        } else if (resp == 0x04U) {
            ack = DAP_TRANSFER_OK;
        } else {
            ack = DAP_TRANSFER_ERROR;
        }

        if (ack != DAP_TRANSFER_OK) {
            PIN_SWDIO_OUT_ENABLE();
            SW_WRITE_BIT(0U);
            SW_CLOCK_CYCLE();
            PIN_SWDIO_OUT_DISABLE();
        }

        if ((request & DAP_TRANSFER_RnW) && (ack == DAP_TRANSFER_OK)) {
            parity = 0U;
            val = 0U;
            for (uint32_t i = 0; i < 32; i++) {
                SW_READ_BIT(resp);
                val |= (resp << i);
                parity ^= resp;
            }
            SW_READ_BIT(resp);
            if ((resp & 1U) != parity) {
                ack = DAP_TRANSFER_ERROR;
            }
            if (data != NULL) {
                *data = val;
            }
        }

        PIN_SWDIO_OUT_ENABLE();
        SW_WRITE_BIT(0U);
        SW_CLOCK_CYCLE();
        SW_CLOCK_CYCLE();

        if (ack == DAP_TRANSFER_WAIT) {
            continue;
        }
        break;
    } while (retry-- && !DAP_TransferAbort);

    if (ack == DAP_TRANSFER_WAIT) {
        ack = DAP_TRANSFER_ERROR;
    }

    return (uint8_t)ack;
}

uint8_t SWD_TransferCheck(uint32_t request, uint32_t *data)
{
    uint32_t val;
    uint32_t mask;
    uint32_t retry;

    retry = DAP_Data.transfer.match_retry;
    val = *data;

    do {
        if (DAP_TransferAbort) {
            return DAP_TRANSFER_ERROR;
        }

        uint8_t ack = SWD_Transfer(request, data);
        if (ack != DAP_TRANSFER_OK) {
            return ack;
        }

        mask = DAP_Data.transfer.match_mask;
        if ((val & mask) == (*data & mask)) {
            return DAP_TRANSFER_OK;
        }

        request &= ~DAP_TRANSFER_RnW;
        request |= DAP_TRANSFER_MATCH_VALUE;
    } while (retry-- && !DAP_TransferAbort);

    return DAP_TRANSFER_MISMATCH;
}

#endif /* DAP_SWD */
