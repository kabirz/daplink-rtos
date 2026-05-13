#include <zephyr/kernel.h>
#include <string.h>
#include "swd_host.h"
#include "DAP.h"
#include "DAP_config.h"
#include "debug_cm.h"

#ifndef DP_SELECT
#define DP_SELECT 0x08
#endif

#if (DAP_SWD != 0)

static SWD_CONNECT_TYPE reset_connect = CONNECT_NORMAL;

uint8_t swd_init(void)
{
    swj_gpio_init();
    DAP_Setup();

    /* Configure for SWD mode */
    DAP_Data.debug_port = DAP_PORT_SWD;
    DAP_Data.clock_delay = DAP_CLOCK_DELAY(DAP_DEFAULT_SWJ_CLOCK);

    /* Send SWD sequence to activate SWD mode */
    uint8_t swd_seq[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    SWJ_Sequence(56, swd_seq);

    uint8_t jtag2swd_seq[] = {0x9E, 0xE7};
    SWJ_Sequence(16, jtag2swd_seq);

    /* Send more clocks */
    uint8_t idle_seq[] = {0x00};
    SWJ_Sequence(8, idle_seq);

    return 1;
}

uint8_t swd_off(void)
{
    DAP_Data.debug_port = DAP_PORT_DISABLED;
    return 1;
}

uint8_t swd_init_debug(void)
{
    uint32_t val;
    uint32_t tmp;

    /* Read IDCODE */
    if (!swd_read_dp(0x00, &val)) {
        return 0;
    }

    /* Clear errors */
    swd_write_dp(0x01, 0x1E);

    /* Power up */
    swd_read_dp(0x01, &tmp);
    tmp |= 0x50000000;
    swd_write_dp(0x01, tmp);

    /* Request debug breakpoint */
    swd_read_dp(0x01, &tmp);
    tmp |= 0x14000000;
    swd_write_dp(0x01, tmp);

    /* Unlock access to AP */
    swd_write_ap(0x00, 0xC5ACCE55);

    return 1;
}

uint8_t swd_clear_errors(void)
{
    uint32_t val;
    swd_read_dp(0x00, &val);
    swd_write_dp(0x01, 0x1E);
    return 1;
}

uint8_t swd_read_dp(uint8_t adr, uint32_t *val)
{
    uint32_t request = (adr & 0x0F) | DAP_TRANSFER_RnW;
    uint8_t ret = SWD_Transfer(request, val);
    return (ret == DAP_TRANSFER_OK) ? 1 : 0;
}

uint8_t swd_write_dp(uint8_t adr, uint32_t val)
{
    uint32_t request = (adr & 0x0F);
    uint8_t ret = SWD_Transfer(request, &val);
    return (ret == DAP_TRANSFER_OK) ? 1 : 0;
}

uint8_t swd_read_ap(uint32_t adr, uint32_t *val)
{
    uint32_t request = (adr & 0x0F) | DAP_TRANSFER_APnDP | DAP_TRANSFER_RnW;
    uint8_t ret;

    /* Select AP register */
    ret = SWD_Transfer(request, NULL);
    if (ret != DAP_TRANSFER_OK) return 0;

    /* Read with dummy to get data */
    ret = SWD_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, val);
    return (ret == DAP_TRANSFER_OK) ? 1 : 0;
}

uint8_t swd_write_ap(uint32_t adr, uint32_t val)
{
    uint32_t request = (adr & 0x0F) | DAP_TRANSFER_APnDP;
    uint8_t ret = SWD_Transfer(request, &val);
    return (ret == DAP_TRANSFER_OK) ? 1 : 0;
}

uint8_t swd_read_word(uint32_t addr, uint32_t *val)
{
    uint32_t request;
    uint8_t ret;

    /* Set AP TAR */
    request = (0x04 & 0x0F) | DAP_TRANSFER_APnDP;
    ret = SWD_Transfer(request, &addr);
    if (ret != DAP_TRANSFER_OK) return 0;

    /* Read DRW */
    request = (0x0C & 0x0F) | DAP_TRANSFER_APnDP | DAP_TRANSFER_RnW;
    ret = SWD_Transfer(request, NULL);
    if (ret != DAP_TRANSFER_OK) return 0;

    /* Get data */
    ret = SWD_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, val);
    return (ret == DAP_TRANSFER_OK) ? 1 : 0;
}

uint8_t swd_write_word(uint32_t addr, uint32_t val)
{
    uint32_t request;
    uint8_t ret;

    /* Set AP TAR */
    request = (0x04 & 0x0F) | DAP_TRANSFER_APnDP;
    ret = SWD_Transfer(request, &addr);
    if (ret != DAP_TRANSFER_OK) return 0;

    /* Write DRW */
    request = (0x0C & 0x0F) | DAP_TRANSFER_APnDP;
    ret = SWD_Transfer(request, &val);
    return (ret == DAP_TRANSFER_OK) ? 1 : 0;
}

uint8_t swd_read_byte(uint32_t addr, uint8_t *val)
{
    uint32_t word;
    if (!swd_read_word(addr & ~0x03, &word)) return 0;
    *val = (uint8_t)(word >> ((addr & 0x03) * 8));
    return 1;
}

uint8_t swd_write_byte(uint32_t addr, uint8_t val)
{
    uint32_t word;
    if (!swd_read_word(addr & ~0x03, &word)) return 0;
    uint32_t shift = (addr & 0x03) * 8;
    word &= ~(0xFF << shift);
    word |= ((uint32_t)val << shift);
    return swd_write_word(addr & ~0x03, word);
}

uint8_t swd_read_memory(uint32_t address, uint8_t *data, uint32_t size)
{
    while (size >= 4) {
        if (!swd_read_word(address, (uint32_t *)data)) return 0;
        address += 4;
        data += 4;
        size -= 4;
    }
    while (size > 0) {
        if (!swd_read_byte(address, data)) return 0;
        address++;
        data++;
        size--;
    }
    return 1;
}

uint8_t swd_write_memory(uint32_t address, uint8_t *data, uint32_t size)
{
    while (size >= 4) {
        if (!swd_write_word(address, *(uint32_t *)data)) return 0;
        address += 4;
        data += 4;
        size -= 4;
    }
    while (size > 0) {
        if (!swd_write_byte(address, *data)) return 0;
        address++;
        data++;
        size--;
    }
    return 1;
}

uint8_t swd_read_core_register(uint32_t n, uint32_t *val)
{
    /* Select register via DHCSDR */
    if (!swd_write_word(0xE000EDF4, n)) return 0;

    /* Read data via DCDRSR */
    if (!swd_read_word(0xE000EDF8, val)) return 0;

    return 1;
}

uint8_t swd_write_core_register(uint32_t n, uint32_t val)
{
    /* Select register via DHCSDR */
    if (!swd_write_word(0xE000EDF4, n)) return 0;

    /* Write data via DCDRDR */
    if (!swd_write_word(0xE000EDF8, val)) return 0;

    return 1;
}

uint8_t swd_set_target_state_hw(target_state_t state)
{
    switch (state) {
        case TARGET_STATE_RESET_RUN:
            nreset_out(0);
            k_busy_wait(100);
            nreset_out(1);
            k_busy_wait(100);
            break;
        case TARGET_STATE_HALT:
            swd_init_debug();
            break;
        default:
            break;
    }
    return 1;
}

uint8_t swd_set_target_state_sw(target_state_t state)
{
    return swd_set_target_state_hw(state);
}

uint8_t swd_transfer_retry(uint32_t req, uint32_t *data)
{
    uint8_t ret;
    uint32_t retry = DAP_Data.transfer.retry_count;

    do {
        ret = SWD_Transfer(req, data);
    } while ((ret == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);

    return ret;
}

void swd_set_reset_connect(SWD_CONNECT_TYPE type)
{
    reset_connect = type;
}

uint8_t JTAG2SWD(void)
{
    uint8_t seq1[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    SWJ_Sequence(56, seq1);

    uint8_t seq2[] = {0x9E, 0xE7};
    SWJ_Sequence(16, seq2);

    uint8_t seq3[] = {0x00};
    SWJ_Sequence(8, seq3);

    return 1;
}

static uint8_t swd_write_debug_state(DEBUG_STATE *state)
{
    if (!swd_write_dp(DP_SELECT, 0)) return 0;
    for (uint32_t i = 0; i < 4; i++) {
        if (!swd_write_core_register(i, state->r[i])) return 0;
    }
    if (!swd_write_core_register(9, state->r[9])) return 0;
    for (uint32_t i = 13; i < 16; i++) {
        if (!swd_write_core_register(i, state->r[i])) return 0;
    }
    if (!swd_write_core_register(16, state->xpsr)) return 0;

    swd_write_word(DBG_HCSR, DBGKEY | C_DEBUGEN | C_MASKINTS | C_HALT);
    swd_write_word(DBG_HCSR, DBGKEY | C_DEBUGEN | C_MASKINTS);

    uint32_t status;
    if (!swd_read_dp(DP_CTRL_STAT, &status)) return 0;
    if (status & 0x00000020) return 0;
    return 1;
}

static uint8_t swd_wait_until_halted(void)
{
    uint32_t val;
    for (uint32_t i = 0; i < MAX_TIMEOUT; i++) {
        if (!swd_read_word(DBG_HCSR, &val)) return 0;
        if (val & S_HALT) return 1;
    }
    return 0;
}

uint8_t swd_flash_syscall_exec(const program_syscall_t *sysCallParam,
    uint32_t entry, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4,
    flash_algo_return_t return_type)
{
    DEBUG_STATE state;
    memset(&state, 0, sizeof(state));
    state.r[0]  = arg1;
    state.r[1]  = arg2;
    state.r[2]  = arg3;
    state.r[3]  = arg4;
    state.r[9]  = sysCallParam->static_base;
    state.r[13] = sysCallParam->stack_pointer;
    state.r[14] = sysCallParam->breakpoint;
    state.r[15] = entry;
    state.xpsr  = 0x01000000;

    if (!swd_write_debug_state(&state)) return 0;
    if (!swd_wait_until_halted()) return 0;
    if (!swd_read_core_register(0, &state.r[0])) return 0;

    swd_write_word(DBG_HCSR, DBGKEY | C_DEBUGEN | C_HALT);

    if (return_type == FLASHALGO_RETURN_POINTER) {
        if (state.r[0] != (arg1 + arg2)) return 0;
    } else {
        if (state.r[0] != 0) return 0;
    }
    return 1;
}

#endif /* DAP_SWD */
