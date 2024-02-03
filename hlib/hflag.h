#ifndef HLIB_HFLAG_H
#define HLIB_HFLAG_H

#include <stdbool.h>
#include "core.h"
#include "hstring.h"

// Short names are optional, long names, descriptions, and default values aren't
i64* hflag_int(char short_name, char* long_name, char* description, i64 default_value);
str* hflag_str(char short_name, char* long_name, char* description, str default_value);
f64* hflag_float(char short_name, char* long_name, char* description, f64 default_value);
bool* hflag_bool(char short_name, char* long_name, char* description);

void hflag_set_help_intro(char* intro);

void hflag_parse(int* argc_pointer, char*** argv_pointer);
#endif
