#pragma once
#include <stddef.h>

extern void memcpy(void* d, void* src, size_t n);
extern int memcmp(void* a, void* b);
extern void memmove(void* dst, void* src);
extern size_t strlen(char* a);
extern int strcmp(char* a, char* b);
extern void strcpy(char* dst, char* src);