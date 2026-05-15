#include <stdint.h>
#include <stddef.h>

#include <core.h>
#include <builtins.h>
#include <acpi.h>
#include <avmf.h>
#include <pager.h>

void outb(uint16_t port, uint8_t value) {
    __asm__ volatile (
        "outb %0, %1"
        : :
        "a"(value), "Nd"(port)
    );
}

void outw(uint16_t port, uint16_t value) {
    __asm__ volatile (
        "outw %0, %1"
        : : 
        "a"(value), "Nd"(port)
    );
}

void outl(uint16_t port, uint32_t value) {
    __asm__ volatile (
        "outl %0, %1"
        : :
        "a"(value), "Nd"(port)
    );
}

void outsw(uint16_t port, const void *buffer, uint32_t count) {
    const uint16_t *buf = (const uint16_t *)buffer;

    __asm__ volatile (
        "rep outsw"
        :
        "+S"(buf), "+c"(count)
        :
        "d"(port)
        :
        "memory"
    );
}

uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile (
        "inb %1, %0"
        :
        "=a"(value)
        :
        "Nd"(port)
    );
    return value;
}

uint16_t inw(uint16_t port) {
    uint16_t value;
    __asm__ volatile (
        "inw %1, %0"
        :
        "=a"(value)
        :
        "Nd"(port)
    );
    return value;
}

uint32_t inl(uint16_t port) {
    uint32_t value;
    __asm__ volatile (
        "inl %1, %0"
        :
        "=a"(value)
        :
        "Nd"(port)
    );
    return value;
}

void insw(uint16_t port, void *buffer, uint32_t count) {
    uint16_t *buf = (uint16_t *)buffer;

    __asm__ volatile (
        "rep insw"
        :
        "+D"(buf), "+c"(count)
        :
        "d"(port)
        :
        "memory"
    );
}


void spin_lock(spinlock_t* lock) {
    while (__sync_lock_test_and_set(lock, 1)) {
        while (*lock);
    }
}

void spin_unlock(spinlock_t* lock) {
    __sync_lock_release(lock);
}

unsigned long long spin_lock_irqsave(spinlock_t* lock) {
    unsigned long long flags = 0;
    asm volatile(
        "pushfq\n\t"
        "pop %0\n\t"
        "cli"
        :
        "=r"(flags)
        : :
        "memory"
    );
    while (__sync_lock_test_and_set(lock, 1)) {
        while (*lock);
    }

    return flags;
}

void spin_unlock_irqrestore(spinlock_t* lock, unsigned long long flags) {
    __sync_lock_release(lock);
    asm volatile(
        "push %0\n\t"
        "popfq"
        : :
        "r"(flags)
        :
        "memory", "cc"
    );
}


unsigned int str_to_uint(const char* str) {
    unsigned int result = 0;
    if (!str) return 0;

    // Hex prefix?
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
        while (*str) {
            char c = *str++;
            result <<= 4;
            if (c >= '0' && c <= '9') result += c - '0';
            else if (c >= 'a' && c <= 'f') result += c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') result += c - 'A' + 10;
            else break; // stop at first invalid char
        }
    } else {
        while (*str >= '0' && *str <= '9') {
            result = result * 10 + (*str - '0');
            str++;
        }
    }

    return result;
}

static uint64_t tsc_ticks_per_ms = 0;
static uint8_t rdtscp_supported = 0;

static uint64_t ftimer_read_tsc(void) {
    uint32_t low = 0;
    uint32_t high = 0;
    if (rdtscp_supported == 1) {
        asm volatile("rdtscp" : "=a"(low), "=d"(high) : : "rcx");
        asm volatile("cpuid" : : : "rax", "rcx", "rdx", "memory");
    }
    else {
        asm volatile("cpuid" : : : "rax", "rcx", "rdx", "memory");
        asm volatile("rdtsc" : "=a"(low), "=d"(high) : : "memory");
    }
    return ((uint64_t)high << 32) | low;
}

void ftimer_calibrate(void) {
    uint32_t eax, ebx, ecx, edx;
    eax = 0x80000001;
    asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(eax));
    if (edx & (1 << 27)) {
        rdtscp_supported = 1;
    } else {
        rdtscp_supported = 0;
    }

    uint64_t start = ftimer_read_tsc();
    acpi_mdelay(10);
    uint64_t end = ftimer_read_tsc();
    uint64_t cycles_per_10ms = end - start;
    tsc_ticks_per_ms = cycles_per_10ms / 10;
}

void fdelay(uint32_t ms) {
    if (tsc_ticks_per_ms == 0) return;

    uint64_t start = ftimer_read_tsc();
    uint64_t ticks_needed = (uint64_t)ms * tsc_ticks_per_ms;

    while ((ftimer_read_tsc() - start) < ticks_needed) {
        asm volatile("pause");
    }
}

#define ALLOC_HDR_SIG "PFI_ALLOC\0"
#define ALLOC_HDR_SIG_LEN 10

struct alloc_hdr {
    char sig[ALLOC_HDR_SIG_LEN];
    size_t size;
    struct alloc_hdr* nxt;
    struct alloc_hdr* prev;
} __attribute__((packed));

void* fmalloc(size_t size) {
    void* out = (void*)avmf_alloc(size + sizeof(struct alloc_hdr), MALLOC_TYPE_USER, PAGE_PRESENT | PAGE_RW, NULL);
    if (!out) return out;
    void* ret = out + sizeof(struct alloc_hdr);
    
    struct alloc_hdr* hdr = (struct alloc_hdr*)out;
    
    memcpy(hdr->sig, ALLOC_HDR_SIG, ALLOC_HDR_SIG_LEN);
    hdr->size = size;
    hdr->nxt = NULL;
    hdr->prev = NULL;

    return ret;
}

void flink(void* ptr1, void* ptr2) {
    struct alloc_hdr* hdr1 = (struct alloc_hdr*)(ptr1 - sizeof(struct alloc_hdr));
    struct alloc_hdr* hdr2 = (struct alloc_hdr*)(ptr2 - sizeof(struct alloc_hdr));
    
    hdr1->nxt = hdr2;
    hdr2->prev = hdr1;
}

void ffree(void* ptr) {
    struct alloc_hdr* hdr = (struct alloc_hdr*)(ptr - sizeof(struct alloc_hdr));
    while (hdr != NULL) {
        if (memcmp(hdr->sig, ALLOC_HDR_SIG, ALLOC_HDR_SIG_LEN) != 0) break;

        uint64_t free_ptr = (uint64_t)hdr;
        hdr = hdr->nxt;
        avmf_free(free_ptr);
    }
}

void* fcalloc(size_t nmemb, size_t size) {
    void* out = fmalloc(nmemb * size);
    if (!out) return out;
    memset(out, 0, size * nmemb);
    return out;
}

void* frealloc(void* ptr, size_t new_size) {
    struct alloc_hdr* hdr = (struct alloc_hdr*)(ptr - sizeof(struct alloc_hdr));
    if (memcmp(hdr, ALLOC_HDR_SIG, ALLOC_HDR_SIG_LEN) != 0) return NULL;
    
    void* out = fmalloc(new_size);
    if (!out) return out;

    memcpy(out, ptr, hdr->size);
    ffree(ptr);
    return out;
}
