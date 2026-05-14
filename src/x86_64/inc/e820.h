#pragma once

#include <stdint.h>

#define MAX_E820_ENTRIES 128

#define E820_TYPE_RAM 1
#define E820_TYPE_RESERVED 2
#define E820_TYPE_ACPI_RECLAIM 3
#define E820_TYPE_ACPI_NVS 4
#define E820_TYPE_BAD 5

struct e820_entry {
    uint64_t base;
    uint64_t len;
    uint32_t type;
    uint32_t ext;
} __attribute__((packed));

struct e820 {
    uint32_t entry_count;
    struct e820_entry entries[MAX_E820_ENTRIES];
} __attribute__((packed));

void init_basic_e820(void) __attribute__((used));
struct e820* get_e820(void) __attribute__((used));
