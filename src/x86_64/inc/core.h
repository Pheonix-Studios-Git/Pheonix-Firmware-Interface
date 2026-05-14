#pragma once

#include <builtins.h>
#include <stdint.h>

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