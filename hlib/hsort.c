#include "core.h"
#include "hsort.h"
#include <stdbool.h>

void memswap(void* a, void* b, usize len) {
	u8* aa = (u8*)a;
	u8* bb = (u8*)b;
	for(usize i = 0; i < len; i++) {
		u8 tmp = *(aa + i);
		*(aa + i) = *(bb + i);
		*(bb + i) = tmp;
	}
}

usize median_of_3_pivot(void* data, usize len, usize element_size, i32 (*cmp)(void* a, void* b)) {
	usize a_index = 0;
	usize b_index = len/2;
	usize c_index = len-1;

	void* a = (u8*)data + a_index*element_size;
	void* b = (u8*)data + b_index*element_size;
	void* c = (u8*)data + c_index*element_size;

	bool a_lt_b = cmp(a, b) < 0;
	bool b_lt_c = cmp(b, c) < 0;
	bool a_lt_c = cmp(a, c) < 0;

	if (a_lt_b && b_lt_c) {
		return b_index;
	}
	if (a_lt_c && !b_lt_c) {
		return c_index;
	}
	if (!a_lt_b && a_lt_c) {
		return a_index;
	}
	return b_index;
}

// quicksort
void hsort(void* data, usize len, usize element_size, i32 (*cmp)(void* a, void* b)) {
	if (len < 2) {
		return;
	}
	u8* dataa = (u8*)data;
	isize pivot = median_of_3_pivot(data, len, element_size, cmp);
	isize low = 0;
	isize high = len-2;

	memswap(dataa + pivot*element_size, dataa + (len-1)*element_size, element_size);
	pivot = len-1;

	while(true) {
		while (cmp(dataa + low*element_size, dataa + pivot*element_size) < 0 && low < (isize)len) {
			low++;
		}
		while (cmp(dataa + high*element_size, dataa + pivot*element_size) > 0 && high >= 0) {
			high--;
		}
		if(low > high) {
			memswap(dataa + low*element_size, dataa + pivot*element_size, element_size);
			pivot = low;
			break;
		}
		memswap(dataa + low*element_size, dataa + high*element_size, element_size);
		low++;
		high--;
	}

	hsort(data, pivot, element_size, cmp);
	hsort((u8*)data + (pivot+1)*element_size, len - pivot - 1, element_size, cmp);
}
