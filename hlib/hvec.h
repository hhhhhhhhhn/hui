#ifndef HLIB_HVEC_H
#define HLIB_HVEC_H

#include "core.h"

typedef struct HVec {
	usize element_size;
	usize len;
	usize cap;
	void* data;
} HVec;

HVec hvec_new_with_cap(usize element_size, usize cap);
HVec hvec_new(usize element_size);
void hvec_push(HVec* vec, void* element);
void hvec_free(HVec* vec);
void hvec_clear(HVec* vec);
void* hvec_at(HVec* vec, usize index);

#endif
