#pragma once

#include <stdint.h>
#include <stddef.h>

void outb(uint16_t port, uint8_t value) __attribute__((used));
void outw(uint16_t port, uint16_t value) __attribute__((used));
void outl(uint16_t port, uint32_t value) __attribute__((used));
void outsw(uint16_t port, const void *buffer, uint32_t count) __attribute__((used));
uint8_t inb(uint16_t port) __attribute__((used));
uint16_t inw(uint16_t port) __attribute__((used));
uint32_t inl(uint16_t port) __attribute__((used));
void insw(uint16_t port, void *buffer, uint32_t count) __attribute__((used));

typedef volatile int spinlock_t __attribute__((aligned(64)));

void spin_lock(spinlock_t* lock) __attribute__((used));
void spin_unlock(spinlock_t* lock) __attribute__((used));
unsigned long long spin_lock_irqsave(spinlock_t* lock) __attribute__((used));
void spin_unlock_irqrestore(spinlock_t* lock, unsigned long long flags) __attribute__((used));

unsigned int str_to_uint(const char* str) __attribute__((used));

void ftimer_calibrate(void) __attribute__((used));
void fdelay(uint32_t ms) __attribute__((used));
void* fmalloc(size_t size) __attribute__((used));
void flink(void* ptr1, void* ptr2) __attribute__((used));
void ffree(void* ptr) __attribute__((used));
void* fcalloc(size_t nmemb, size_t size) __attribute__((used));
void* frealloc(void* ptr, size_t new_size) __attribute__((used));