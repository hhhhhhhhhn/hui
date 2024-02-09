#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "core.h"
#include "hvec.h"

HVec hvec_new_with_cap(usize element_size, usize cap) {
	void* data = malloc(cap * element_size);
	nullpanic(data);
	HVec vec = {
		.element_size = element_size,
		.len = 0,
		.cap = cap,
		.data = data,
	};

	return vec;
}

HVec hvec_new(usize element_size) {
	return hvec_new_with_cap(element_size, 16);
}

// internal
void hvec_resize(HVec* vec, usize new_cap) {
	assert(new_cap >= vec->len);

	void* new_data = realloc(vec->data, new_cap*vec->element_size);
	nullpanic(new_data);
	vec->data = new_data;
	vec->cap = new_cap;
}

void hvec_push(HVec* vec, void* element) {
	if (vec->len >= vec->cap) {
		hvec_resize(vec, vec->cap * 2);
	}
	void* destination = (u8*)vec->data + vec->len*vec->element_size;
	memcpy(destination, element, vec->element_size);
	vec->len++;
}

void hvec_clear(HVec* vec) {
	vec->len = 0;
}

void hvec_free(HVec* vec) {
	free(vec->data);
}

void* hvec_at(HVec* vec, usize index) {
	if (index >= vec->len) {
		return NULL;
	}
	return (u8*)vec->data + index*vec->element_size;
}
