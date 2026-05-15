#include <stdint.h>
#include <stddef.h>

#include <e820.h>

struct e820 g_e820 = {0};

static void add_region(uint64_t base, uint64_t len, uint64_t type, uint64_t ext) {
    if (g_e820.entry_count > MAX_E820_ENTRIES) g_e820.entry_count = 0;
    struct e820_entry* e = &g_e820.entries[g_e820.entry_count++];
    e->base = base;
    e->len = len;
    e->type = type;
    e->ext = ext;
}

void init_basic_e820(void) {
    uint64_t RAM_END = 0x10000000; // Minimum RAM Needed for function - 256MB

    add_region(0x00000000, 0x00300000, E820_TYPE_RESERVED, 0); // Firmware Stack + VGA + Data
    add_region(0x00300000, RAM_END - 0x00300000, E820_TYPE_RAM, 0); // Usable
    add_region(0xE0000000, 0x10000000, E820_TYPE_RESERVED, 0); // PCIe ECAM
    add_region(0xFEC00000, 0x00001000, E820_TYPE_RESERVED, 0); // IOAPIC
    add_region(0xFEE00000, 0x00001000, E820_TYPE_RESERVED, 0); // LAPIC
    add_region(0xFF000000, 0x01000000, E820_TYPE_RESERVED, 0); // Firmware Code
}

struct e820* get_e820(void) {
    return &g_e820;
}
