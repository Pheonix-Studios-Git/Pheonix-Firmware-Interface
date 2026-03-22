#pragma once

#include <stdint.h>

int is_ps2_present(void) __attribute__((used));
void ps2_init(void) __attribute__((used));
int8_t ps2_read_scan(void) __attribute__((used));
int16_t ps2_try_read_scan(void) __attribute__((used));