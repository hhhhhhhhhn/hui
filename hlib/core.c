#include <stdio.h>
#include <stdlib.h>

void* malloc_print(size_t size, const char* file, int line) {
	void* ptr = malloc(size);
	if (!ptr) {
		fprintf(stderr, "[mem]: %s:%d failed to allocate %zu bytes\n", file, line, size);
	}
	fprintf(stderr, "[mem]: %s:%d allocated %zu bytes: %p\n", file, line, size, ptr);
	return ptr;
}

void free_print(void* ptr, const char* file, int line) {
	fprintf(stderr, "[mem]: %s:%d freed %p\n", file, line, ptr);
	free(ptr);
}

void* realloc_print(void* ptr, size_t size, const char* file, int line) {
	fprintf(stderr, "[mem]: %s:%d reallocated %zu bytes from %p", file, line, size, ptr);
	void* new_ptr = realloc(ptr, size);
	if (!new_ptr) {
		fprintf(stderr, " failed to allocate %zu bytes\n", size);
	}
	fprintf(stderr, " to %p\n", new_ptr);
	return new_ptr;
}

