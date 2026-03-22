#pragma once

#include <stdint.h>

struct ATA_DP {
    uint64_t lba;
    uint16_t count;
};

int ata_read_sectors(struct ATA_DP* dp, void* buffer, uint8_t drive) __attribute__((used));
int ata_write_sectors(struct ATA_DP* dp, const void* buffer, uint8_t drive) __attribute__((used));
