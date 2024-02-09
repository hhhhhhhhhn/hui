#include "hparse.h"
#include "core.h"
#include "hstring.h"

// TODO: Make sure the size is right
bool hparse_u64(str view, u64* result) {
	if(view.len == 0) return false;

	*result = 0;
	for(usize i = 0; i < view.len; i++) {
		if(!hstring_is_digit(view.data[i])) return false;
		*result *= 10;
		*result += view.data[i] - '0';
	}
	return true;
}

bool hparse_i64(str view, i64* result) {
	bool neg = false;
	*result = 0;

	if (view.len > 0 && view.data[0] == '+') {
		view.data++; view.len--;
	} else if (view.len > 0 && view.data[0] == '-') {
		neg = true;
		view.data++; view.len--;
	}

	if(view.len == 0) return false;

	for(usize i = 0; i < view.len; i++) {
		if(!hstring_is_digit(view.data[i])) return false;
		*result *= 10;
		*result += view.data[i] - '0';
	}
	if (neg) {
		*result = -*result;
	}
	return true;
}

// TODO: Add NaN and Infinity support
bool hparse_f64(str view, f64* result) {
	bool neg = false;
	*result = 0;
	if (view.len > 0 && view.data[0] == '+') {
		view.data++; view.len--;
	} else if (view.len > 0 && view.data[0] == '-') {
		neg = true;
		view.data++; view.len--;
	}

	bool after_point = false;
	i64 exponent = 0;

	if(view.len == 0) return false;

	for(usize i = 0; i < view.len; i++) {
		if(view.data[i] == '.') {
			if (after_point) return false; // Two points, error
			after_point = true;
			continue;
		}

		if((view.data[i] == 'e' || view.data[i] == 'E') && i > 0) {
			str e_exponent_str = str_slice(view, i+1, view.len);
			i64 e_exponent;
			if (!hparse_i64(e_exponent_str, &e_exponent)) return false;
			exponent += e_exponent;
			break;
		}

		if(!hstring_is_digit(view.data[i])) return false;
		if (after_point) {
			exponent--;
		}
		*result *= 10;
		*result += view.data[i] - '0';
	}
	while(exponent > 0) {
		*result *= 10;
		exponent--;
	}
	while(exponent < 0) {
		*result /= 10;
		exponent++;
	}
	if (neg) {
		*result = -*result;
	}
	return true;
}
