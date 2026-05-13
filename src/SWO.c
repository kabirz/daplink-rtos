#include "DAP_config.h"
#include "DAP.h"

#if ((SWO_UART != 0) || (SWO_MANCHESTER != 0))

#if (SWO_UART != 0)
void SWO_UART_Init(uint32_t baudrate)
{
    (void)baudrate;
}

void SWO_UART_Uninit(void)
{
}

uint32_t SWO_UART_GetBaudrate(void)
{
    return 0;
}

void SWO_UART_Control(uint32_t control)
{
    (void)control;
}

uint8_t swo_uart_in(void)
{
    return 0;
}

void swo_uart_out(uint8_t bit)
{
    (void)bit;
}
#endif

#if (SWO_MANCHESTER != 0)
void SWO_Manchester_Init(uint32_t baudrate)
{
    (void)baudrate;
}

void SWO_Manchester_Uninit(void)
{
}

uint32_t SWO_Manchester_GetBaudrate(void)
{
    return 0;
}

void SWO_Manchester_Control(uint32_t control)
{
    (void)control;
}

uint8_t swo_manchester_in(void)
{
    return 0;
}

void swo_manchester_out(uint8_t bit)
{
    (void)bit;
}
#endif

#endif /* SWO_UART || SWO_MANCHESTER */
