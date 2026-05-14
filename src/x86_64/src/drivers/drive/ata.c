#include <core.h>
#include <ata.h>

static spinlock_t tmp_lock = 0;
static spinlock_t ata_lock = 0;

// I/O port addresses for the primary ATA bus.
#define ATA_PRIMARY_BASE 0x1F0
#define ATA_PRIMARY_CONTROL 0x3F6

// I/O ports relative to the base address
#define ATA_REG_DATA 0x00
#define ATA_REG_ERROR 0x01
#define ATA_REG_SECTOR_COUNT 0x02
#define ATA_REG_LBA_LOW 0x03
#define ATA_REG_LBA_MID 0x04
#define ATA_REG_LBA_HIGH 0x05
#define ATA_REG_DRIVE_HEAD 0x06
#define ATA_REG_COMMAND 0x07
#define ATA_REG_STATUS 0x07

// Status Register Bits
#define ATA_SR_BSY 0x80 // Busy
#define ATA_SR_DRDY 0x40 // Drive Ready
#define ATA_SR_DF 0x20 // Device Fault
#define ATA_SR_DRQ 0x08 // Data Request Ready
#define ATA_SR_ERR 0x01 // Error

// ATA Commands
#define ATA_CMD_READ_PIO_EXT 0x24 // LBA48 Read
#define ATA_CMD_WRITE_PIO_EXT 0x34 // LBA48 Write
#define ATA_CMD_FLUSH_EXT 0xEA // LBA48 Flush

// ATA Ports
#define ATA_PRIMARY_DATA 0x1F0
#define ATA_PRIMARY_ERROR 0x1F1
#define ATA_PRIMARY_SECTOR_COUNT 0x1F2
#define ATA_PRIMARY_LBA_LOW 0x1F3
#define ATA_PRIMARY_LBA_MID 0x1F4
#define ATA_PRIMARY_LBA_HIGH 0x1F5
#define ATA_PRIMARY_DRIVE_HEAD 0x1F6
#define ATA_PRIMARY_COMMAND 0x1F7
#define ATA_PRIMARY_STATUS 0x1F7 // Status register is at the same address as Command
#define ATA_PRIMARY_IO 0x1F0
#define ATA_PRIMARY_CTRL 0x3F6
#define ATA_SECONDARY_IO 0x170
#define ATA_SECONDARY_CTRL 0x376

typedef struct {
    uint16_t io_base;
    uint16_t ctrl_base;
} ata_bus_t;

static const ata_bus_t ata_buses[2] = {
    { ATA_PRIMARY_IO, ATA_PRIMARY_CTRL },
    { ATA_SECONDARY_IO, ATA_SECONDARY_CTRL }
};

static inline void _ata_wait_ready(uint16_t io_base) {
    uint8_t status;
    int timeout = 100000; // ~100ms depending on CPU speed

    while (timeout--) {
        status = inb(io_base + 7);
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRDY))
            return; // Ready!
    }
}

int ata_read_sectors(struct ATA_DP* dp, void* buffer, uint8_t drive) {
    uint64_t rflags = spin_lock_irqsave(&ata_lock);

    uint8_t bus = ((drive - 0x80) >> 1) & 1;
    uint8_t device = (drive & 1); // 0=master,1=slave
    uint16_t io = ata_buses[bus].io_base;
    uint16_t ctrl = ata_buses[bus].ctrl_base;

    _ata_wait_ready(io);

    // Send the LBA48 and sector count high byte first
    outb(ctrl, 0x00); // Clear high order bytes
    outb(io + 2, (dp->count >> 8) & 0xFF);
    outb(io + 3, (dp->lba >> 24) & 0xFF);
    outb(io + 4, (dp->lba >> 32) & 0xFF);
    outb(io + 5, (dp->lba >> 40) & 0xFF);

    // Send the LBA48 low bytes and sector count low byte.
    outb(io + 6, 0x40 | (device << 4)); // Choose drive

    outb(io + 2, dp->count & 0xFF);
    outb(io + 3, dp->lba & 0xFF);
    outb(io + 4, (dp->lba >> 8) & 0xFF);
    outb(io + 5, (dp->lba >> 16) & 0xFF);

    // Issue the LBA48 Read command.
    outb(io + 7, ATA_CMD_READ_PIO_EXT);

    // Read each sector.
    for (int i = 0; i < dp->count; i++) {
        _ata_wait_ready(io);
        uint8_t status = inb(io + 7);

        // Check for errors.
        if (status & ATA_SR_ERR) { // Error
            spin_unlock_irqrestore(&ata_lock, rflags);
            return -1;
        }

        // Wait for Data Request Ready.
        while (!(inb(io + 7) & ATA_SR_DRQ));

        // Read 256 words (512 bytes) into the buffer.
        insw(io, (uint8_t*)buffer + (uint32_t)i * 512, 256);
    }
    spin_unlock_irqrestore(&ata_lock, rflags);

    return 0; // Success
}

int ata_write_sectors(struct ATA_DP* dp, const void* buffer, uint8_t drive) {
    uint64_t rflags = spin_lock_irqsave(&ata_lock);

    uint8_t bus = ((drive - 0x80) >> 1) & 1;
    uint8_t device = (drive & 1);
    uint16_t io = ata_buses[bus].io_base;
    uint16_t ctrl = ata_buses[bus].ctrl_base;

    _ata_wait_ready(io);

    // Same as read
    outb(ctrl, 0x00);
    outb(io + 2, (dp->count >> 8) & 0xFF);
    outb(io + 3, (dp->lba >> 24) & 0xFF);
    outb(io + 4, (dp->lba >> 32) & 0xFF);
    outb(io + 5, (dp->lba >> 40) & 0xFF);

    outb(io + 6, 0x40 | (device << 4));
    outb(io + 2, dp->count & 0xFF);
    outb(io + 3, dp->lba & 0xFF);
    outb(io + 4, (dp->lba >> 8) & 0xFF);
    outb(io + 5, (dp->lba >> 16) & 0xFF);

    // Issue the LBA48 Write command.
    outb(io + 7, ATA_CMD_WRITE_PIO_EXT);

    // Write each sector.
    for (int i = 0; i < dp->count; i++) {
        _ata_wait_ready(io);
        uint8_t status = inb(io + 7);

        if (status & ATA_SR_ERR) {
            spin_unlock_irqrestore(&ata_lock, rflags);
            return -1;
        }

        while (!(inb(io + 7) & ATA_SR_DRQ));

        // Write 256 words (512 bytes) from the buffer.
        outsw(io, (uint8_t*)buffer + (uint32_t)i * 512, 256);
    }

    // Flush the cache to ensure data is written to disk.
    outb(io + 7, ATA_CMD_FLUSH_EXT);
    _ata_wait_ready(io);

    spin_unlock_irqrestore(&ata_lock, rflags);

    return 0; // Success
}
