#ifndef HLIB_HHASHMAP_H
#define HLIB_HHASHMAP_H

#include "core.h"
#include <stdbool.h>

typedef struct HKeyType {
	usize (*hash)(void* key, usize size);
	bool (*eq)(void* key1, void* key2, usize size);
} HKeyType;

// internal. TODO: Hide
typedef struct EntryInfo {
	bool occupied;
	bool deleted;
} EntryInfo;

typedef struct HHashMap {
	usize len;
	usize cap;
	usize key_size;
	usize value_size;
	void* keys;
	void* values;
	EntryInfo* info;
	HKeyType type;
} HHashMap;

HHashMap hhashmap_new_with_cap(usize key_size, usize value_size, HKeyType type, usize cap);
HHashMap hhashmap_new(usize key_size, usize value_size, HKeyType type);
bool hhashmap_next(HHashMap* map, void* ret_key, void* ret_value, usize* index); // ret_key and ret_value are TYPE**, see hashmap_test
void hhashmap_set(HHashMap* map, void* key, void* value);
void* hhashmap_get(HHashMap* map, void* key);
void hhashmap_delete(HHashMap* map, void* key);
void hhashmap_free(HHashMap* map);

bool hkeytype_direct_eq(void* key1, void* key2, usize size);
usize hkeytype_direct_hash(void* key, usize size);
extern const HKeyType HKEYTYPE_DIRECT;

#endif
