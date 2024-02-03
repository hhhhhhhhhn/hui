#ifndef HLIB_CORE_H
#define HLIB_CORE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uintptr_t usize;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef intptr_t isize;

typedef float f32;
typedef double f64;

// char and bool are also used in hlib

void* malloc_print(usize size, const char* file, int line);
void free_print(void* ptr, const char* file, int line);
void* realloc_print(void* ptr, usize size, const char* file, int line);

#ifdef HLIB_DEBUG
#define malloc(size) malloc_print(size, __FILE__, __LINE__)
#define free(ptr) free_print(ptr, __FILE__, __LINE__)
#define realloc(ptr, size) realloc_print(ptr, size, __FILE__, __LINE__)
#endif

#define nullpanic(ptr) do {if (!(ptr)) {fprintf(stderr, "Unexpected null at %s:%d\n", __FILE__, __LINE__); exit(1);}} while (0);
#define todo() do {fprintf(stderr, "Not implemented at %s:%d\n", __FILE__, __LINE__); exit(1);} while(0);
#define unreachable() do {fprintf(stderr, "Unreachable at %s:%d\n", __FILE__, __LINE__); exit(1);} while(0);
#define panic(str) do {fprintf(stderr, "Panic: " str " at %s:%d\n", __FILE__, __LINE__); exit(1);} while(0);
#define panicf(str, ...) do {fprintf(stderr, "Panic: " str " at %s:%d\n", __VA_ARGS__, __FILE__, __LINE__); exit(1);} while(0);
#define assert(expr) do {if (!(expr)) {fprintf(stderr, "Assertion '%s' failed at %s:%d\n", #expr, __FILE__, __LINE__); exit(1);}} while (0);

#define ARCH_BYTES (sizeof(usize))
#define ARCH_BITS (sizeof(usize)*8)

#define PLATFORM_LINUX   0
#define PLATFORM_MAC     1
#define PLATFORM_WINDOWS 2

#if defined(WIN32)
	#define PLATFORM PLATFORM_WINDOWS
#elif defined(__APPLE__)
	#define PLATFORM PLATFORM_MAC
#elif defined(__linux__)
	#define PLATFORM PLATFORM_LINUX
#else
	#error Unsupported platform, please update "hlib/core.h"
#endif

#endif
