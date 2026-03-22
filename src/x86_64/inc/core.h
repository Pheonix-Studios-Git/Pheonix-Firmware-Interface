#pragma once

#define outb(value, port) __asm__ volatile("outb %0, %w1" : : "a"((unsigned char)value), "Nd"((unsigned short)port))
#define outw(value, port) __asm__ volatile("outw %0, %w1" : : "a"((unsigned short)value), "Nd"((unsigned short)port))
#define outl(value, port) __asm__ volatile("outl %0, %w1" : : "a"((unsigned long)value), "Nd"((unsigned short)port))
#define outsw(port, buffer, count) __asm__ volatile("rep outsw" : "+S"((unsigned short*)buffer), "+c"((unsigned int)count) : "d"((unsigned short)port) : "memory")
#define inb(port) ({ \
    unsigned char __core_usgae_inb_v; \
    __asm__ volatile("inb %w1, %0" : "=a"(__core_usgae_inb_v) : "Nd"((unsigned short)port)); \
    __core_usgae_inb_v; \
})
#define inw(port) ({ \
    unsigned short __core_usgae_inw_v; \
    __asm__ volatile("inw %w1, %0" : "=a"(__core_usgae_inw_v) : "Nd"((unsigned short)port)); \
    __core_usage_inw_v; \
})
#define inl(port) ({ \
    unsigned long __core_usgae_inl_v; \
    __asm__ volatile("inl %w1, %0" : "=a"(__core_usgae_inl_v) : "Nd"((unsigned short)port)); \
    __core_usgae_inl_v; \
})
#define insw(port, buffer, count) __asm__ volatile("rep insw" : "+D"((unsigned short*)buffer), "+c"((unsigned int)count) : "d"((unsigned short)port) : "memory")

typedef volatile int spinlock_t;

void spin_lock(spinlock_t* lock) __attribute__((used));
void spin_unlock(spinlock_t* lock) __attribute__((used));
unsigned long long spin_lock_irqsave(spinlock_t* lock) __attribute__((used));
void spin_unlock_irqrestore(spinlock_t* lock, unsigned long long flags) __attribute__((used));

unsigned int str_to_uint(const char* str) __attribute__((used));