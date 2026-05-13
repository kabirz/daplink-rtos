#ifndef DEBUG_CM_H
#define DEBUG_CM_H

#define DBG_HCSR        0xE000EDF0
#define DBG_DSR         0xE000EDF4
#define DHCSR           0xE000EDF0
#define DCRSR           0xE000EDF4
#define DCRDR           0xE000EDF8
#define DEMCR           0xE000EDFC

/* DHCSR bits */
#define DBGKEY          (0xA05F << 16)
#define C_DEBUGEN       (1 << 0)
#define C_HALT          (1 << 1)
#define C_STEP          (1 << 2)
#define C_MASKINTS      (1 << 3)
#define S_HALT          (1 << 17)
#define S_REGRDY        (1 << 16)

/* DCRSR bits */
#define REGWnR          (1 << 16)

/* DEMCR bits */
#define VC_CORERESET    (1 << 0)

#define MAX_TIMEOUT     10000000

typedef struct {
    uint32_t r[16];
    uint32_t xpsr;
} DEBUG_STATE;

#endif
