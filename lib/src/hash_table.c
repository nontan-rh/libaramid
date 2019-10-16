#include <aramid/aramid.h>

#include "hash_table.h"

struct TAG_ARMD__HashTableNode {
    ARMD__HashTableNode *next;
    ARMD_Handle key;
    void *value;
};

ARMD__HashTable *armd__hash_table_create(ARMD_MemoryRegion *memory_region,
                                         ARMD_Size initial_table_size,
                                         ARMD_Real rehash_ratio) {
    if (memory_region == NULL) {
        return NULL;
    }

    if (initial_table_size == 0) {
        return NULL;
    }

    if (!(rehash_ratio > 0)) {
        return NULL;
    }

    int hash_table_initialized = 0;
    int table_initialized = 0;
    ARMD__HashTableNode *table = NULL;

    ARMD__HashTable *hash_table =
        armd_memory_region_allocate(memory_region, sizeof(ARMD__HashTable));
    if (hash_table == NULL) {
        goto error;
    }
    hash_table_initialized = 1;

    table = armd_memory_region_allocate(
        memory_region, sizeof(ARMD__HashTableNode) * initial_table_size);
    if (table == NULL) {
        goto error;
    }
    table_initialized = 1; // NOLINT(clang-analyzer-deadcode.DeadStores)

    for (ARMD_Size i = 0; i < initial_table_size; i++) {
        table[i].next = NULL;
    }

    hash_table->memory_region = memory_region;
    hash_table->table_size = initial_table_size;
    hash_table->num_entries = 0;
    hash_table->rehash_ratio = rehash_ratio;
    hash_table->table = table;

    return hash_table;

error:
    if (hash_table_initialized) {
        armd_memory_region_free(memory_region, hash_table);
    }

    if (table_initialized) {
        armd_memory_region_free(memory_region, table);
    }

    return NULL;
}

int armd__hash_table_destroy(ARMD__HashTable *hash_table) {
    int node_destroyed = 0;

    ARMD_MemoryRegion *memory_region = hash_table->memory_region;

    for (ARMD_Size i = 0; i < hash_table->table_size; i++) {
        ARMD__HashTableNode *sentinel_node = &hash_table->table[i];
        ARMD__HashTableNode *node = sentinel_node->next;
        while (node != NULL) {
            ARMD__HashTableNode *next_node = node->next;

            armd_memory_region_free(memory_region, node);
            node_destroyed = 1;

            node = next_node;
        }
    }

    armd_memory_region_free(memory_region, hash_table->table);
    armd_memory_region_free(memory_region, hash_table);

    return node_destroyed;
}

ARMD_Bool armd__hash_table_is_empty(const ARMD__HashTable *hash_table) {
    return hash_table->num_entries == 0;
}

ARMD_Size armd__hash_table_get_num_entries(const ARMD__HashTable *hash_table) {
    return hash_table->num_entries;
}

ARMD__HashTableNode *create_node(ARMD_MemoryRegion *memory_region,
                                 ARMD_Handle key, void *value,
                                 ARMD__HashTableNode *next) {
    ARMD__HashTableNode *node =
        armd_memory_region_allocate(memory_region, sizeof(ARMD__HashTableNode));
    if (node == NULL) {
        return NULL;
    }

    node->key = key;
    node->value = value;
    node->next = next;

    return node;
}

static int rehash_if_needed(ARMD__HashTable *hash_table) {
    ARMD_MemoryRegion *memory_region = hash_table->memory_region;

    if (hash_table->rehash_ratio * hash_table->table_size >
        hash_table->num_entries) {
        return 0;
    }

    ARMD_Size old_table_size = hash_table->table_size;
    ARMD_Size new_table_size = old_table_size * 2 + 1;

    ARMD__HashTableNode *old_table = hash_table->table;
    ARMD__HashTableNode *new_table = armd_memory_region_allocate(
        memory_region, sizeof(ARMD__HashTableNode) * new_table_size);

    if (new_table == NULL) {
        return -1;
    }

    for (ARMD_Size i = 0; i < new_table_size; i++) {
        new_table[i].next = NULL;
    }

    for (ARMD_Size old_table_index = 0; old_table_index < old_table_size;
         old_table_index++) {
        ARMD__HashTableNode *node = old_table[old_table_index].next;
        while (node != NULL) {
            ARMD__HashTableNode *next_node = node->next;

            ARMD_Size new_table_index = node->key % new_table_size;
            ARMD__HashTableNode *new_sentinel_node =
                &new_table[new_table_index];
            node->next = new_sentinel_node->next;
            new_sentinel_node->next = node;

            node = next_node;
        }
    }

    armd_memory_region_free(memory_region, old_table);

    hash_table->table = new_table;
    hash_table->table_size = new_table_size;

    return 0;
}

int armd__hash_table_insert(ARMD__HashTable *hash_table, ARMD_Handle key,
                            void *value) {
    if (rehash_if_needed(hash_table) != 0) {
        return -1;
    }

    ARMD_MemoryRegion *memory_region = hash_table->memory_region;

    ARMD_Size table_index = key % hash_table->table_size;

    ARMD__HashTableNode *sentinel_node = &hash_table->table[table_index];

    ARMD__HashTableNode *node = sentinel_node->next;
    for (; node != NULL; node = node->next) {
        if (node->key == key) {
            return -1;
        }
    }

    ARMD__HashTableNode *new_node =
        create_node(memory_region, key, value, sentinel_node->next);
    if (new_node == NULL) {
        return -1;
    }

    new_node->key = key;
    new_node->value = value;
    new_node->next = sentinel_node->next;
    sentinel_node->next = new_node;

    ++hash_table->num_entries;

    return 0;
}

int armd__hash_table_update(ARMD__HashTable *hash_table, ARMD_Handle key,
                            void *new_value, void **old_value) {
    ARMD_Size table_index = key % hash_table->table_size;

    ARMD__HashTableNode *sentinel_node = &hash_table->table[table_index];

    ARMD__HashTableNode *node = sentinel_node->next;
    for (; node != NULL; node = node->next) {
        if (node->key == key) {
            *old_value = node->value;
            node->value = new_value;
            return 0;
        }
    }

    *old_value = NULL;
    return -1;
}

int armd__hash_table_upsert(ARMD__HashTable *hash_table, ARMD_Handle key,
                            void *new_value, void **old_value) {
    if (rehash_if_needed(hash_table) != 0) {
        return -1;
    }

    ARMD_MemoryRegion *memory_region = hash_table->memory_region;

    ARMD_Size table_index = key % hash_table->table_size;

    ARMD__HashTableNode *sentinel_node = &hash_table->table[table_index];

    ARMD__HashTableNode *node = sentinel_node->next;
    for (; node != NULL; node = node->next) {
        if (node->key == key) {
            *old_value = node->value;
            node->value = new_value;
            return 0;
        }
    }

    ARMD__HashTableNode *new_node =
        create_node(memory_region, key, new_value, sentinel_node->next);
    if (new_node == NULL) {
        return -1;
    }

    new_node->key = key;
    new_node->value = new_value;
    new_node->next = sentinel_node->next;
    sentinel_node->next = new_node;

    ++hash_table->num_entries;

    *old_value = NULL;
    return 1;
}

ARMD_Bool armd__hash_table_exists(ARMD__HashTable *hash_table,
                                  ARMD_Handle key) {
    ARMD_Size table_index = key % hash_table->table_size;

    ARMD__HashTableNode *sentinel_node = &hash_table->table[table_index];

    ARMD__HashTableNode *node = sentinel_node->next;
    for (; node != NULL; node = node->next) {
        if (node->key == key) {
            return 1;
        }
    }

    return 0;
}

int armd__hash_table_get(ARMD__HashTable *hash_table, ARMD_Handle key,
                         void **value) {
    ARMD_Size table_index = key % hash_table->table_size;

    ARMD__HashTableNode *sentinel_node = &hash_table->table[table_index];

    ARMD__HashTableNode *node = sentinel_node->next;
    for (; node != NULL; node = node->next) {
        if (node->key == key) {
            *value = node->value;
            return 0;
        }
    }

    *value = NULL;
    return -1;
}

int armd__hash_table_remove(ARMD__HashTable *hash_table, ARMD_Handle key) {
    ARMD_MemoryRegion *memory_region = hash_table->memory_region;

    ARMD_Size table_index = key % hash_table->table_size;

    ARMD__HashTableNode *sentinel_node = &hash_table->table[table_index];

    ARMD__HashTableNode *prev_node = sentinel_node;
    ARMD__HashTableNode *node = sentinel_node->next;
    for (; node != NULL; prev_node = node, node = node->next) {
        if (node->key == key) {
            prev_node->next = node->next;
            armd_memory_region_free(memory_region, node);

            --hash_table->num_entries;
            return 0;
        }
    }

    return -1;
}
