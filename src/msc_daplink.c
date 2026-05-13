#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(msc_daplink, LOG_LEVEL_INF);

#define DISK_NAME "DAPLINK"

/* FAT12 layout: 64 sectors, 512B/sector, 1 sector/cluster */
#define SECTOR_SIZE     512
#define SECTORS_TOTAL   64
#define RESERVED_SECTORS 1
#define FAT_COUNT       2
#define ROOT_ENTRIES    16
#define SECTORS_PER_FAT 1
#define SECTORS_PER_CLUSTER 1

#define FAT_START       1
#define ROOT_START      (FAT_START + FAT_COUNT * SECTORS_PER_FAT)
#define DATA_START      (ROOT_START + 1)

#define CLUSTER_COUNT   (SECTORS_TOTAL - DATA_START)

static const char details_content[] =
    "DAPLink on Zephyr RTOS\n"
    "Board: black_f407ve\n"
    "HIC: STM32F407VET6\n"
    "Firmware: CMSIS-DAP v2.1.0\n";

static const char fw_hex_content[] = "";

static void build_boot_sector(uint8_t buf[SECTOR_SIZE])
{
    memset(buf, 0, SECTOR_SIZE);

    buf[0] = 0xEB;  buf[1] = 0x3C;  buf[2] = 0x90;  /* jmp + nop */

    memcpy(&buf[3], "DAPLINK ", 8);

    /* BPB */
    buf[11]  = SECTOR_SIZE & 0xFF;          /* BytesPerSector lo */
    buf[12]  = (SECTOR_SIZE >> 8) & 0xFF;   /* BytesPerSector hi */
    buf[13]  = SECTORS_PER_CLUSTER;         /* SectorsPerCluster */
    buf[14]  = RESERVED_SECTORS;            /* ReservedSectors */
    buf[15]  = FAT_COUNT;                   /* NumberOfFATs */
    buf[16]  = ROOT_ENTRIES & 0xFF;         /* RootEntries lo */
    buf[17]  = (ROOT_ENTRIES >> 8) & 0xFF;  /* RootEntries hi */
    buf[18]  = SECTORS_TOTAL & 0xFF;        /* TotalSectors lo */
    buf[19]  = (SECTORS_TOTAL >> 8) & 0xFF; /* TotalSectors hi */
    buf[21]  = SECTORS_PER_FAT;             /* SectorsPerFAT */

    buf[26]  = 0x29;                        /* Extended boot sig */
    buf[27]  = 0x01;  buf[28] = 0x02;  buf[29] = 0x03;  buf[30] = 0x04; /* serial */
    memcpy(&buf[31], "DAPLINK     ", 11);   /* Volume label */
    memcpy(&buf[42], "FAT12   ", 8);         /* FAT type */

    buf[510] = 0x55;  buf[511] = 0xAA;      /* Boot signature */
}

static void build_fat(uint8_t buf[SECTOR_SIZE], int is_primary)
{
    memset(buf, 0, SECTOR_SIZE);

    buf[0] = 0xF0;          /* Media descriptor */
    buf[1] = 0xFF;
    buf[2] = 0xFF;          /* Cluster 1: EOC (0xFFF) */

    int fw_hex_size = sizeof(fw_hex_content) - 1;
    int detail_clusters = (sizeof(details_content) - 1 + SECTOR_SIZE - 1) / SECTOR_SIZE;
    int fw_clusters = (fw_hex_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    int cluster = 2;

    /* DETAILS.TXT cluster chain */
    for (int i = 0; i < detail_clusters; i++) {
        int next = (i < detail_clusters - 1) ? (cluster + 1) : 0xFFF;
        int idx = cluster * 3 / 2;
        if (cluster & 1) {
            buf[idx] = (buf[idx] & 0x0F) | ((next & 0x0F) << 4);
            buf[idx + 1] = (next >> 4) & 0xFF;
        } else {
            buf[idx] = next & 0xFF;
            buf[idx + 1] = (buf[idx + 1] & 0xF0) | ((next >> 8) & 0x0F);
        }
        cluster++;
    }

    /* FIRMWARE.HEX cluster chain */
    if (fw_clusters == 0) {
        int idx = cluster * 3 / 2;
        if (cluster & 1) {
            buf[idx] = (buf[idx] & 0x0F) | (0xFF);
            buf[idx + 1] = 0xFF;
        } else {
            buf[idx] = 0xFF;
            buf[idx + 1] = (buf[idx + 1] & 0xF0) | 0x0F;
        }
    } else {
        for (int i = 0; i < fw_clusters; i++) {
            int next = (i < fw_clusters - 1) ? (cluster + 1) : 0xFFF;
            int idx = cluster * 3 / 2;
            if (cluster & 1) {
                buf[idx] = (buf[idx] & 0x0F) | ((next & 0x0F) << 4);
                buf[idx + 1] = (next >> 4) & 0xFF;
            } else {
                buf[idx] = next & 0xFF;
                buf[idx + 1] = (buf[idx + 1] & 0xF0) | ((next >> 8) & 0x0F);
            }
            cluster++;
        }
    }

    if (!is_primary) {
        buf[0] = 0xF0;
        buf[1] = 0xFF;
        buf[2] = 0xFF;
    }
}

static void build_root_dir(uint8_t buf[SECTOR_SIZE])
{
    memset(buf, 0, SECTOR_SIZE);

    /* DETAILS.TXT */
    memcpy(&buf[0], "DETAILS  TXT", 11);
    buf[11] = 0x20;
    buf[13] = 0xD7;
    buf[14] = 0xAC;
    buf[15] = 0xD7;
    buf[16] = 0xAC;
    buf[22] = 0xD7;
    buf[23] = 0xAC;
    buf[24] = 0xD7;
    buf[25] = 0xAC;
    buf[26] = sizeof(details_content) - 1;
    buf[27] = (sizeof(details_content) - 1) >> 8;
    int start_cluster = 2;
    buf[26] = (sizeof(details_content) - 1) & 0xFF;
    buf[27] = ((sizeof(details_content) - 1) >> 8) & 0xFF;
    buf[28] = start_cluster & 0xFF;
    buf[29] = (start_cluster >> 8) & 0xFF;

    /* FIRMWARE.HEX */
    int fw_size = sizeof(fw_hex_content) - 1;
    memcpy(&buf[32], "FIRMWARE HEX", 11);
    buf[43] = 0x20;
    buf[45] = 0xD7;
    buf[46] = 0xAC;
    buf[47] = 0xD7;
    buf[48] = 0xAC;
    buf[54] = 0xD7;
    buf[55] = 0xAC;
    buf[56] = 0xD7;
    buf[57] = 0xAC;
    buf[58] = fw_size & 0xFF;
    buf[59] = (fw_size >> 8) & 0xFF;
    buf[60] = 3 & 0xFF;          /* next free cluster */
    buf[61] = (3 >> 8) & 0xFF;
}

static int write_sector(int sector, const uint8_t *data)
{
    return disk_access_write(DISK_NAME, data, sector, 1);
}

static int format_disk(void)
{
    uint8_t sector[SECTOR_SIZE];

    if (disk_access_init(DISK_NAME)) return -1;

    build_boot_sector(sector);
    if (write_sector(0, sector)) return -1;

    build_fat(sector, 1);
    if (write_sector(FAT_START, sector)) return -1;

    build_fat(sector, 0);
    if (write_sector(FAT_START + 1, sector)) return -1;

    build_root_dir(sector);
    if (write_sector(ROOT_START, sector)) return -1;

    memset(sector, 0, SECTOR_SIZE);
    memcpy(sector, details_content, sizeof(details_content) - 1);
    if (write_sector(DATA_START, sector)) return -1;

    memset(sector, 0, SECTOR_SIZE);
    memcpy(sector, fw_hex_content, sizeof(fw_hex_content) - 1);
    if (write_sector(DATA_START + 1, sector)) return -1;

    return 0;
}

static int daplink_disk_init(void)
{
    int ret = format_disk();
    if (ret) {
        LOG_ERR("Disk format failed");
    } else {
        LOG_INF("DAPLink disk formatted (32KB FAT12)");
    }
    return ret;
}

SYS_INIT(daplink_disk_init, APPLICATION, 1);
