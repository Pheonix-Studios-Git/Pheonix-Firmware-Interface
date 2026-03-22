#pragma once

#include <stdint.h>

void serial_init(void) __attribute__((used));
void serial_printc(char c) __attribute__((used));
void serial_print(const char* str) __attribute__((used));
int serial_is_transmit_empty(void) __attribute__((used));
void serial_printf(const char* fmt, ...) __attribute__((used));