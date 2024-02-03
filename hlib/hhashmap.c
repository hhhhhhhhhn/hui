#include <stdbool.h>
#include <string.h>
#include "hhashmap.h"
#include "core.h"

HHashMap hhashmap_new_with_cap(usize key_size, usize value_size, HKeyType type, usize cap) {
	void* keys = malloc(cap * key_size);
	nullpanic(keys);
	void* values = malloc(cap * value_size);
	nullpanic(values);
	EntryInfo* info = malloc(cap * sizeof(EntryInfo));
	nullpanic(info);
	memset(info, 0, cap*sizeof(EntryInfo));

	return (HHashMap) {
		.len = 0,
		.cap = cap,
		.key_size = key_size,
		.value_size = value_size,
		.keys = keys,
		.values = values,
		.type = type,
		.info = info,
	};
}

HHashMap hhashmap_new(usize key_size, usize value_size, HKeyType type) {
	return hhashmap_new_with_cap(key_size, value_size, type, 128);
}

void hhashmap_free(HHashMap* map) {
	free(map->keys);
	free(map->values);
	free(map->info);
}

// Index must start as 0, out_key and out_value are pointers to pointers to the type
bool hhashmap_next(HHashMap* map, void* out_key, void* out_value, usize* index) {
	void** ret_key = out_key;
	void** ret_value = out_value;
	while(*index < map->cap && (!map->info[*index].occupied || map->info[*index].deleted)) {
		(*index)++;
	}
	if (*index >= map->cap) {
		return false;
	}
	*ret_key = (char*)map->keys + *index*map->key_size;
	*ret_value = (char*)map->values + *index*map->value_size;

	(*index)++;
	return true;
}

void hhashmap_set(HHashMap* map, void* key, void* value);
// internal
void hhashmap_grow(HHashMap* map) {
	HHashMap new_map = hhashmap_new_with_cap(map->key_size, map->value_size, map->type, map->cap*2);
	void* key;
	void* value;
	usize index = 0;

	while(hhashmap_next(map, &key, &value, &index)) {
		hhashmap_set(&new_map, key, value);
	}

	hhashmap_free(map);

	*map = new_map;
}

void hhashmap_set(HHashMap* map, void* key, void* value) {
	if (4*map->len > 3*map->cap) { // A 0.75 load factor
		hhashmap_grow(map);
	}
	usize index = map->type.hash(key, map->key_size) % map->cap;

	for(usize safety = 0; safety < map->len+1; safety++) {
		if (!map->info[index].occupied || map->info[index].deleted) {
			map->info[index].occupied = true;
			map->info[index].deleted = false;
			memcpy((char*)map->keys + index*map->key_size, key, map->key_size);
			memcpy((char*)map->values + index*map->value_size, value, map->value_size);
			map->len++;
			return;
		}
		index = (index + 1) % map->cap;
	}
	unreachable();
}

// Returns -1 if not found
// internal
int hhashmap_get_index(HHashMap* map, void* key) {
	usize hash = map->type.hash(key, map->key_size);
	int index = hash % map->cap;

	for(usize safety = 0; safety < map->cap; safety++) {
		if (!map->info[index].occupied) {
			break;
		}
		if (map->info[index].deleted) {
			index++;
			continue;
		}
		if (map->type.eq(key, (char*)map->keys + index*map->key_size, map->key_size)) {
			return index;
		}
		index = (index + 1) % map->cap;
	}
	return -1;
}

// Returns NULL if not found
void* hhashmap_get(HHashMap* map, void* key) {
	int index = hhashmap_get_index(map, key);
	if (index == -1) {
		return NULL;
	}
	return (char*)map->values + index*map->value_size;
}

// Cannot fail
void hhashmap_delete(HHashMap* map, void* key) {
	int index = hhashmap_get_index(map, key);

	if (index == -1) {
		return;
	}

	map->info[index].deleted = true;
	map->len--;
}

bool hkeytype_direct_eq(void* key1, void* key2, usize size) {
	return memcmp(key1, key2, size) == 0;
}

// sdbm
usize hkeytype_direct_hash(void* key, usize size) {
	usize hash = 0;
	for (usize i = 0; i < size; i++) {
		hash = ((char*)key)[i] + (hash << 6) + (hash << 16) - hash;
	}
	return hash;
}

const HKeyType HKEYTYPE_DIRECT = {
	.eq = hkeytype_direct_eq,
	.hash = hkeytype_direct_hash,
};
