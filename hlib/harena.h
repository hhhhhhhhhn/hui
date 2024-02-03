#ifndef HLIB_HARENA_H
#define HLIB_HARENA_H

#include "core.h"

typedef struct HStaticArena {
	void* data;
	usize cap;
	usize used;
} HStaticArena;

HStaticArena hstaticarena_new(usize cap);
void* hstaticarena_alloc(HStaticArena* arena, usize size);
void hstaticarena_free(HStaticArena* arena);

#define ARENA_AMOUNT 30

typedef struct HArena {
	HStaticArena sarenas[ARENA_AMOUNT];
	usize sarenas_used;
	usize cap;
} HArena;

HArena harena_new_with_cap(usize cap);
HArena harena_new();
void* harena_alloc(HArena* arena, usize size);
void harena_free(HArena* arena);

#endif
