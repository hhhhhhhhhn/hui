#ifndef HLIB_HSTRING_H
#define HLIB_HSTRING_H

#include <stdbool.h>
#include "core.h"

// Represents immutable view into a string, does not allocate
typedef struct str {
	char* data;
	usize len;
} str;

typedef struct strb {
	char* data;
	usize len;
	usize cap;
} strb;

typedef struct {
	strb builder;
	bool ok;
} strbResult;

#define STR(string) ((str) { .data = (string), .len = sizeof(string) - 1 }) // For static strings
#define STR_ARR(string) { .data = (string), .len = sizeof(string) - 1 } // Usage for static arrays/struct

str str_new(char* data, usize len);
str str_from_strb(strb* builder);
char* str_to_cstr(str view); // Resulting string must be freed
void str_to_cstr_buf(str view, char* buf, usize buf_cap); // Panics if buf cannot hold view
str str_from_cstr(char* cstr);
str str_slice(str view, usize start, usize end);
void str_consume_chars(str* view, usize count);
void str_consume_char(str* view);
str str_trim_left(str view);
str str_trim_right(str view);
bool str_starts_with(str view, str prefix);
bool str_ends_with(str view, str prefix);
str str_remove_start(str view, str prefix);
str str_remove_end(str view, str prefix);
str str_trim(str view);
str str_split_char(str* view, char delim);
str str_split_str(str* view, str delim);
str str_split_while_predicate(str* view, bool(*pred)(char));
bool str_eq(str a, str b);
bool str_write_to_file(str* view, FILE* file);
bool str_write_to_filepath(str* view, str path);

bool hstring_is_whitespace(char c);
bool hstring_is_digit(char c);

strb strb_new();
strb strb_from_str(str view); // Clones the data
strbResult strb_from_filepath(str path);
strbResult strb_from_file(FILE* file);
void strb_append_view(strb* builder, str view); // Clones the data
bool strb_append_file(strb* builder, FILE* file); // Returns true on success
bool strb_append_filepath(strb* builder, str path);
void strb_push_char(strb* builder, char c);
void strb_free(strb* builder);

#endif
