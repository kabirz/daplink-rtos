#include <string.h>
#include "info.h"
#include "DAP_config.h"

void info_init(void)
{
}

uint8_t DAP_GetVendorString(char *str)
{
    const char *s = DAP_VENDOR_NAME;
    uint8_t len = (uint8_t)strlen(s);
    memcpy(str, s, len);
    return len;
}

uint8_t DAP_GetProductString(char *str)
{
    const char *s = DAP_PRODUCT_NAME;
    uint8_t len = (uint8_t)strlen(s);
    memcpy(str, s, len);
    return len;
}

uint8_t DAP_GetSerNumString(char *str)
{
    const char *s = DAP_SER_NUM;
    uint8_t len = (uint8_t)strlen(s);
    memcpy(str, s, len);
    return len;
}

uint8_t DAP_GetTargetDeviceVendorString(char *str)
{
    const char *s = DAP_DEVICE_VENDOR;
    uint8_t len = (uint8_t)strlen(s);
    memcpy(str, s, len);
    return len;
}

uint8_t DAP_GetTargetDeviceNameString(char *str)
{
    const char *s = DAP_DEVICE_NAME;
    uint8_t len = (uint8_t)strlen(s);
    memcpy(str, s, len);
    return len;
}

uint8_t DAP_GetBoardVendorString(char *str)
{
    const char *s = DAP_BOARD_VENDOR;
    uint8_t len = (uint8_t)strlen(s);
    memcpy(str, s, len);
    return len;
}

uint8_t DAP_GetBoardNameString(char *str)
{
    const char *s = DAP_BOARD_NAME;
    uint8_t len = (uint8_t)strlen(s);
    memcpy(str, s, len);
    return len;
}

uint8_t DAP_GetProductFirmwareVersionString(char *str)
{
    return 0;
}
