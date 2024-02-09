#ifndef HLIB_HPARSE_H
#define HLIB_HPARSE_H

#include "core.h"
#include "hstring.h"

bool hparse_u64(str view, u64* result);
bool hparse_i64(str view, i64* result);
bool hparse_f64(str view, f64* result);

#endif
