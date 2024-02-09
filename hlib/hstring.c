#include "core.h"
#include "hfs.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "hstring.h"

str str_new(char* data, usize len) {
	return (str) {
		.data = data,
		.len = len
	};
}

str str_from_strb(strb* builder) {
	return (str) {
		.data = builder->data,
		.len = builder->len
	};
}

str str_from_cstr(char* str) {
	usize len = strlen(str);
	return str_new(str, len);
}

char* str_to_cstr(str view) {
	char* result = malloc(view.len + 1);
	nullpanic(result);
	memcpy(result, view.data, view.len);
	result[view.len] = '\0';
	return result;
}

void str_to_cstr_buf(str view, char* buf, usize buf_cap) {
	if (view.len+1 >= buf_cap) {
		panic("Buffer too small to hold string");
	}
	memcpy(buf, view.data, view.len);
	buf[view.len] = '\0';
}

str str_slice(str view, usize start, usize end) {
	return (str) {
		.data = view.data + start,
		.len = end - start
	};
}

void str_consume_chars(str* view, usize count) {
	view->data += count;
	view->len -= count;
}

void str_consume_char(str* view) {
	view->data++;
	view->len--;
}

str str_split_char(str* view, char delim) {
	for(usize i = 0; i < view->len; i++) {
		if (view->data[i] == delim) {
			str result = str_slice(*view, 0, i);
			str_consume_chars(view, i+1);
			return result;
		}
	}

	str result = str_slice(*view, 0, view->len);
	str_consume_chars(view, view->len);
	return result;
}

str str_split_str(str* view, str delim) {
	usize chars_matched = 0;
	for(usize i = 0; i < view->len; i++) {
		while (chars_matched < delim.len && i+chars_matched < view->len
			&& view->data[i+chars_matched] == delim.data[chars_matched]) {
				chars_matched++;
				if(chars_matched == delim.len) {
					str result = str_slice(*view, 0, i);
					str_consume_chars(view, i+chars_matched);
					return result;
				}
		}
		chars_matched = 0;
	}

	str result = str_slice(*view, 0, view->len);
	str_consume_chars(view, view->len);
	return result;
}

str str_split_while_predicate(str* view, bool(*pred)(char)) {
	usize i = 0;
	while (!pred(view->data[i]) && i < view->len) {
		i++;
	}
	str result = str_slice(*view, 0, i);

	while (pred(view->data[i]) && i < view->len) {
		i++;
	}
	str_consume_chars(view, i);

	return result;
}

bool str_starts_with(str view, str prefix) {
	if (prefix.len > view.len) {
		return false;
	}
	return memcmp(view.data, prefix.data, prefix.len) == 0;
}

bool str_ends_with(str view, str suffix) {
	if (suffix.len > view.len) {
		return false;
	}
	return memcmp(view.data + view.len - suffix.len, suffix.data, suffix.len) == 0;
}

str str_remove_start(str view, str prefix) {
	if (str_starts_with(view, prefix)) {
		return str_slice(view, prefix.len, view.len);
	}
	return view;
}

str str_remove_end(str view, str suffix) {
	if (str_ends_with(view, suffix)) {
		return str_slice(view, 0, view.len - suffix.len);
	}
	return view;
}

str str_trim_left(str view) {
	for(usize i = 0; i < view.len; i++) {
		if (!hstring_is_whitespace(view.data[i])) {
			return str_slice(view, i, view.len);
		}
	}
	return str_slice(view, view.len, view.len);
}

str str_trim_right(str view) {
	for(isize i = view.len-1; i >= 0; i--) {
		if (!hstring_is_whitespace(view.data[i])) {
			return str_slice(view, 0, i+1);
		}
	}
	return str_slice(view, 0, 0);
}

str str_trim(str view) {
	return str_trim_right(str_trim_left(view));
}

bool str_eq(str a, str b) {
	if (a.len != b.len) {
		return false;
	}
	return memcmp(a.data, b.data, a.len) == 0;
}

bool str_write_to_file(str* view, FILE* file) {
	int written = fprintf(file, "%.*s", (int)view->len, view->data);;
	return (usize)written == view->len;
}

bool str_write_to_filepath(str* view, str path) {
	FILE* file = hfs_open_file(path, false, true);
	if (file == NULL) {
		return false;
	}
	bool status = str_write_to_file(view, file);
	bool close_status = fclose(file);
	assert(close_status);
	return status;
}

bool hstring_is_whitespace(char c) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool hstring_is_digit(char c) {
	return c >= '0' && c <= '9';
}

/////////////////////// BUILDER /////////////////////////////
strb strb_new() {
	char* data = malloc(32);
	nullpanic(data);

	return (strb) {
		.data = data,
		.len = 0,
		.cap = 32
	};
}

// internal
void strb_resize(strb* builder, usize new_cap) {
	builder->cap = new_cap;
	char* new_data = realloc(builder->data, new_cap);
	nullpanic(new_data);
	builder->data = new_data;
}

void strb_append_view(strb* builder, str view) {
	while (builder->len + view.len >= builder->cap) {
		strb_resize(builder, builder->cap*2); // TODO: Calculate how many times to resize beforehand
	}
	memcpy(builder->data + builder->len, view.data, view.len);
}

bool strb_append_file(strb* builder, FILE* file) {
	while (true) {
		if (builder->len >= builder->cap) {
			strb_resize(builder, builder->cap*2);
		}
		usize bytes_read = fread(builder->data + builder->len, 1, builder->cap - builder->len, file);
		if (ferror(file)) {
			return false;
		}
		builder->len += bytes_read;

		if (bytes_read == 0) {
			break;
		}
	}
	return true;
}

bool strb_append_filepath(strb* builder, str path) {
	FILE* file = hfs_open_file(path, true, false);
	if (file == NULL) {
		return false;
	}
	if (!strb_append_file(builder, file)) {
		fclose(file);
		return false;
	}
	if (fclose(file) != 0) {
		return false;
	}
	return true;
}

strb strb_from_str(str view) {
	strb builder = strb_new();
	strb_append_view(&builder, view);
	return builder;
}

strbResult strb_from_file(FILE* file) {
	strbResult result = {0};
	result.builder = strb_new();
	if(!strb_append_file(&result.builder, file)) {
		strb_free(&result.builder);
		return result;
	}
	result.ok = 1;
	return result;
}

strbResult strb_from_filepath(str path) {
	strbResult result = {0};
	result.builder = strb_new();
	if(!strb_append_filepath(&result.builder, path)) {
		strb_free(&result.builder);
		return result;
	}
	result.ok = 1;
	return result;
}

void strb_push_char(strb* builder, char c) {
	if (builder->len >= builder->cap) {
		strb_resize(builder, builder->cap*2);
	}
	builder->data[builder->len] = c;
	builder->len++;
}

void strb_free(strb* builder) {
	free(builder->data);
}
