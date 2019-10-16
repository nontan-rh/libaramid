#ifndef ARAMID__HASH_TABLE_H
#define ARAMID__HASH_TABLE_H

#include <aramid/aramid.h>

#include "memory_region.h"

typedef struct TAG_ARMD__HashTableNode ARMD__HashTableNode;

typedef struct TAG_ARMD__HashTable {
    ARMD_MemoryRegion *memory_region;
    ARMD_Size table_size;
    ARMD_Size num_entries;
    ARMD_Real rehash_ratio;
    ARMD__HashTableNode *table;
} ARMD__HashTable;

ARMD_EXTERN_C ARMD__HashTable *
armd__hash_table_create(ARMD_MemoryRegion *memory_region,
                        ARMD_Size initial_table_size, ARMD_Real rehash_ratio);
ARMD_EXTERN_C int armd__hash_table_destroy(ARMD__HashTable *hash_table);

ARMD_EXTERN_C ARMD_Bool
armd__hash_table_is_empty(const ARMD__HashTable *hash_table);

ARMD_EXTERN_C ARMD_Size
armd__hash_table_get_num_entries(const ARMD__HashTable *hash_table);

ARMD_EXTERN_C int armd__hash_table_insert(ARMD__HashTable *hash_table,
                                          ARMD_Handle key, void *value);
ARMD_EXTERN_C int armd__hash_table_update(ARMD__HashTable *hash_table,
                                          ARMD_Handle key, void *value,
                                          void **old_value);
ARMD_EXTERN_C int armd__hash_table_upsert(ARMD__HashTable *hash_table,
                                          ARMD_Handle key, void *value,
                                          void **old_value);

ARMD_EXTERN_C ARMD_Bool armd__hash_table_exists(ARMD__HashTable *hash_table,
                                                ARMD_Handle key);
ARMD_EXTERN_C int armd__hash_table_get(ARMD__HashTable *hash_table,
                                       ARMD_Handle key, void **value);

ARMD_EXTERN_C int armd__hash_table_remove(ARMD__HashTable *hash_table,
                                          ARMD_Handle key);

#endif // ARAMID__HASH_TABLE_H
