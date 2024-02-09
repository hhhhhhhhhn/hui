#include "core.h"
#include "harena.h"
#include <stdlib.h>

HStaticArena hstaticarena_new(usize cap) {
	void* data = malloc(cap);
	nullpanic(data);
	return (HStaticArena) {
		.data = data,
		.cap = cap,
		.used = 0,
	};
}

// Returns NULL if out of memory
void* hstaticarena_alloc(HStaticArena* arena, usize size) {
	if (size > arena->cap - arena->used) {
		return NULL;
	}
	void* ptr = ((u8*)arena->data) + arena->used;
	arena->used += size;

	return ptr;
}

void hstaticarena_clear(HStaticArena* arena) {
	arena->used = 0;
}

void hstaticarena_free(HStaticArena* arena) {
	free(arena->data);
}

#define ARENA_AMOUNT 30 // Can handle about 1TB, should be enough
						// every next arena is double the last

HArena harena_new_with_cap(usize cap) {
	HArena arena = {.sarenas = {{0}}, .first_sarena_cap = cap, .sarenas_used = 0};
	arena.sarenas[0] = hstaticarena_new(cap);
	arena.sarenas_used++;
	return arena;
}

HArena harena_new() {
	return harena_new_with_cap(1024);
}

void* harena_alloc(HArena* arena, usize size) {
	for (usize i = arena->sarenas_used-1; i < ARENA_AMOUNT; i++) {
		void* ptr = hstaticarena_alloc(&arena->sarenas[i], size);
		if (ptr != NULL) {
			return ptr;
		}
		assert(i+1 < ARENA_AMOUNT);  // More than 1TB of data? That or memory allocation failed

		if (arena->sarenas[i+1].data == NULL) { // If cleared, this check makes sure we reuse the previously existing one
			arena->sarenas[i+1] = hstaticarena_new(arena->first_sarena_cap * (1 << arena->sarenas_used));
		}
		arena->sarenas_used++;
	}

	unreachable();
}

void harena_clear(HArena* arena) {
	for (usize i = 0; i < arena->sarenas_used; i++) {
		hstaticarena_clear(&arena->sarenas[i]);
	}
	arena->sarenas_used = 1;
}

void harena_free(HArena* arena) {
	for (usize i = 0; i < ARENA_AMOUNT; i++) {
		if (arena->sarenas[i].data == NULL) {
			return;
		}
		hstaticarena_free(&arena->sarenas[i]);
	}
}
