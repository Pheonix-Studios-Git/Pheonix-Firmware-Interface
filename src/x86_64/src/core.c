#include <core.h>

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