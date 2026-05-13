#include <zephyr/kernel.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "intelhex.h"
#include "flash_blob.h"
#include "debug_cm.h"
#include "swd_host.h"

LOG_MODULE_REGISTER(flash_prog, LOG_LEVEL_INF);

#define DISK_NAME       "DAPLINK"
#define SECTOR_SIZE     512
#define DATA_START      4
#define ROOT_DIR        3
#define FW_ENTRY        1
#define DIR_ENTRY_SZ    32

extern const program_target_t * const flash_algo_table[];

static uint32_t last_fw_size;
static uint16_t last_fw_cluster;

static int rd_sector(int s, uint8_t *b) { return disk_access_read(DISK_NAME, b, s, 1); }
static int rd_cluster(int c, uint8_t *b) { return disk_access_read(DISK_NAME, b, DATA_START + c - 2, 1); }

static int get_fw_info(uint32_t *size, uint16_t *cluster)
{
    uint8_t sec[SECTOR_SIZE], ent[DIR_ENTRY_SZ];
    if (rd_sector(ROOT_DIR, sec)) return -1;
    memcpy(ent, sec + FW_ENTRY * DIR_ENTRY_SZ, DIR_ENTRY_SZ);
    if (ent[0] == 0 || ent[0] == 0xE5) return -1;
    *size = ent[28] | ((uint32_t)ent[29] << 8) | ((uint32_t)ent[30] << 16) | ((uint32_t)ent[31] << 24);
    *cluster = ent[26] | ((uint16_t)ent[27] << 8);
    return 0;
}

static int fw_new(void)
{
    uint32_t sz; uint16_t cl;
    if (get_fw_info(&sz, &cl)) return 0;
    if (sz == 0) return 0;
    if (sz != last_fw_size || cl != last_fw_cluster) {
        last_fw_size = sz; last_fw_cluster = cl; return 1;
    }
    return 0;
}

static int read_fw(uint8_t *buf, uint32_t max)
{
    uint8_t sec[SECTOR_SIZE];
    uint32_t off = 0;
    int cl = last_fw_cluster;
    while (cl >= 2 && cl < 0xFF8 && off < max) {
        if (rd_cluster(cl, sec)) return -1;
        uint32_t cp = (max - off) > SECTOR_SIZE ? SECTOR_SIZE : (max - off);
        memcpy(buf + off, sec, cp); off += cp;
        if (rd_sector(1, sec)) return -1;
        int fo = cl * 3 / 2;
        cl = (cl & 1) ? (sec[fo] >> 4) | (sec[fo + 1] << 4) : sec[fo] | ((sec[fo + 1] & 0x0F) << 8);
    }
    return (int)off;
}

static int flash_init(const program_target_t *ft)
{
    swd_init_debug();
    swd_write_word(DBG_HCSR, DBGKEY | C_DEBUGEN | C_HALT);
    for (uint32_t i = 0; i < ft->algo_size; i += 4) {
        uint32_t v;
        memcpy(&v, (const uint8_t *)ft->algo_blob + i, 4);
        if (!swd_write_word(ft->algo_start + i, v)) return -1;
    }
    return swd_flash_syscall_exec(&ft->sys_s, ft->init, 0,0,0,0, FLASHALGO_RETURN_BOOL) ? 0 : -1;
}

static void flash_uninit(const program_target_t *ft)
{
    swd_flash_syscall_exec(&ft->sys_s, ft->uninit, 0,0,0,0, FLASHALGO_RETURN_BOOL);
    swd_write_word(DBG_HCSR, DBGKEY | C_DEBUGEN);
}

static int pg_page(const program_target_t *ft, uint32_t addr, const uint8_t *d, uint32_t sz)
{
    for (uint32_t i = 0; i < sz; i += 4) {
        uint32_t v; memcpy(&v, d + i, 4);
        if (!swd_write_word(ft->program_buffer + i, v)) return -1;
    }
    return swd_flash_syscall_exec(&ft->sys_s, ft->program_page, addr, sz, ft->program_buffer, 0, FLASHALGO_RETURN_BOOL) ? 0 : -1;
}

static int er_sec(const program_target_t *ft, uint32_t addr)
{
    return swd_flash_syscall_exec(&ft->sys_s, ft->erase_sector, addr, 0,0,0, FLASHALGO_RETURN_BOOL) ? 0 : -1;
}

static int prog_bin(const program_target_t *ft, const uint8_t *bin, uint32_t ba, uint32_t bs)
{
    uint32_t sec_sz = 0x400, pg_sz = 0x100;
    uint32_t base = ba & ~(sec_sz - 1), end = ba + bs, off = 0;
    while (base < end) {
        if (er_sec(ft, base)) return -1;
        for (uint32_t p = 0; p < sec_sz; p += pg_sz) {
            uint32_t pa = base + p;
            if (pa >= ba && off < bs) {
                uint32_t l = (bs - off) < pg_sz ? (bs - off) : pg_sz;
                if (pg_page(ft, pa, bin + off, l)) return -1;
                off += l;
            }
        }
        base += sec_sz;
    }
    return 0;
}

void flash_prog_periodic(void)
{
    if (!fw_new()) return;
    LOG_INF("FW detected: %u bytes", last_fw_size);
    if (last_fw_size > 16384) { LOG_ERR("too big"); return; }

    static uint8_t hex[16384], bin[1024];
    uint32_t nr = last_fw_size < sizeof(hex) ? last_fw_size : sizeof(hex);
    if (read_fw(hex, nr) < 0) { LOG_ERR("read fail"); return; }

    reset_hex_parser();
    uint32_t parsed = 0;
    hexfile_parse_status_t st = HEX_PARSE_OK;
    if (flash_algo_table[0] == NULL) { LOG_ERR("no flash algo"); return; }
    const program_target_t *ft = flash_algo_table[0];

    if (flash_init(ft)) { LOG_ERR("flash init fail"); return; }

    while (parsed < nr) {
        uint32_t pcnt, ba, bcnt;
        st = parse_hex_blob(hex + parsed, nr - parsed, &pcnt, bin, sizeof(bin), &ba, &bcnt);
        if (st == HEX_PARSE_OK || st == HEX_PARSE_UNALIGNED) {
            if (prog_bin(ft, bin, ba, bcnt)) { LOG_ERR("prog fail"); break; }
            parsed += pcnt;
            if (st == HEX_PARSE_UNALIGNED) {
                memmove(hex, hex + parsed, nr - parsed);
                nr -= parsed; parsed = 0;
            }
        } else break;
        if (st == HEX_PARSE_EOF) break;
    }

    flash_uninit(ft);
    if (st == HEX_PARSE_EOF) LOG_INF("Programming OK");
    else LOG_ERR("Programming fail");
}
