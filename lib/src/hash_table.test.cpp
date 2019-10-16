#include <gtest/gtest.h>

#include <aramid/aramid.h>

#include "hash_table.h"

namespace {

class HashTableCreationTest : public ::testing::Test {
protected:
    ARMD_MemoryAllocator memory_allocator;
    ARMD_MemoryRegion *memory_region;

    HashTableCreationTest() {}

    ~HashTableCreationTest() override {}

    void SetUp() override {
        armd_memory_allocator_init_default(&memory_allocator);
        memory_region = armd_memory_region_create(&memory_allocator);
    }

    void TearDown() override { armd_memory_region_destroy(memory_region); }
};

TEST_F(HashTableCreationTest, CreateAndDestroyHashTable) {
    int res;
    ARMD__HashTable *hash_table =
        armd__hash_table_create(memory_region, 1, 0.5);
    ASSERT_NE(hash_table, nullptr);
    res = armd__hash_table_destroy(hash_table);
    ASSERT_EQ(res, 0);
}

TEST_F(HashTableCreationTest, CreateZeroSizedHashTable) {
    ARMD__HashTable *hash_table =
        armd__hash_table_create(memory_region, 0, 0.5);
    ASSERT_EQ(hash_table, nullptr);
}

TEST_F(HashTableCreationTest, CreateZeroHashRatioHashTable) {
    ARMD__HashTable *hash_table = armd__hash_table_create(memory_region, 0, 0);
    ASSERT_EQ(hash_table, nullptr);
}

TEST_F(HashTableCreationTest, CreateNegativeHashRatioHashTable) {
    ARMD__HashTable *hash_table =
        armd__hash_table_create(memory_region, 0, -1.0);
    ASSERT_EQ(hash_table, nullptr);
}

TEST_F(HashTableCreationTest, CreateNaNHashRatioHashTable) {
    ARMD__HashTable *hash_table = armd__hash_table_create(
        memory_region, 0, std::numeric_limits<double>::quiet_NaN());
    ASSERT_EQ(hash_table, nullptr);
}

TEST_F(HashTableCreationTest, DestroyNonEmptyHashTable) {
    int res;

    ARMD__HashTable *hash_table =
        armd__hash_table_create(memory_region, 1, 0.5);
    ASSERT_NE(hash_table, nullptr);

    res = armd__hash_table_insert(hash_table, 1, reinterpret_cast<void *>(1));
    ASSERT_EQ(res, 0);

    res = armd__hash_table_destroy(hash_table);
    ASSERT_NE(res, 0);
}

class HashTableTest : public ::testing::Test {
protected:
    ARMD_MemoryAllocator memory_allocator;
    ARMD_MemoryRegion *memory_region;
    ARMD__HashTable *hash_table;

    HashTableTest() {}

    ~HashTableTest() override {}

    void SetUp() override {
        armd_memory_allocator_init_default(&memory_allocator);
        memory_region = armd_memory_region_create(&memory_allocator);
        hash_table = armd__hash_table_create(memory_region, 4, 0.5);
    }

    void TearDown() override {
        armd__hash_table_destroy(hash_table);
        armd_memory_region_destroy(memory_region);
    }
};

TEST_F(HashTableTest, CheckIsEmpty) {
    int res;
    ASSERT_TRUE(armd__hash_table_is_empty(hash_table));

    res = armd__hash_table_insert(hash_table, 1, reinterpret_cast<void *>(1));
    ASSERT_EQ(res, 0);
    ASSERT_FALSE(armd__hash_table_is_empty(hash_table));

    res = armd__hash_table_remove(hash_table, 1);
    ASSERT_EQ(res, 0);
    ASSERT_TRUE(armd__hash_table_is_empty(hash_table));
}

TEST_F(HashTableTest, CheckGetNumEntries) {
    int res;
    ASSERT_EQ(armd__hash_table_get_num_entries(hash_table), 0u);

    res = armd__hash_table_insert(hash_table, 1, reinterpret_cast<void *>(1));
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__hash_table_get_num_entries(hash_table), 1u);
    res = armd__hash_table_insert(hash_table, 2, reinterpret_cast<void *>(2));
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__hash_table_get_num_entries(hash_table), 2u);

    res = armd__hash_table_remove(hash_table, 1);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__hash_table_get_num_entries(hash_table), 1u);
    res = armd__hash_table_remove(hash_table, 2);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(armd__hash_table_get_num_entries(hash_table), 0u);
}

TEST_F(HashTableTest, CheckInsertWithExistentKey) {
    int res;
    void *data;
    ASSERT_EQ(armd__hash_table_get_num_entries(hash_table), 0u);

    res = armd__hash_table_insert(hash_table, 1, reinterpret_cast<void *>(1));
    ASSERT_EQ(res, 0);
    ASSERT_TRUE(armd__hash_table_exists(hash_table, 1));
    ASSERT_EQ(armd__hash_table_get_num_entries(hash_table), 1u);
    res = armd__hash_table_get(hash_table, 1, &data);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data, reinterpret_cast<void *>(1));
    res = armd__hash_table_insert(hash_table, 1, reinterpret_cast<void *>(2));
    ASSERT_NE(res, 0);

    res = armd__hash_table_get(hash_table, 1, &data);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data, reinterpret_cast<void *>(1));

    res = armd__hash_table_remove(hash_table, 1);
    ASSERT_EQ(res, 0);
    ASSERT_FALSE(armd__hash_table_exists(hash_table, 1));
    ASSERT_EQ(armd__hash_table_get_num_entries(hash_table), 0u);
}

TEST_F(HashTableTest, CheckUpsert) {
    int res;
    void *data;
    ASSERT_EQ(armd__hash_table_get_num_entries(hash_table), 0u);

    res = armd__hash_table_upsert(hash_table, 1, reinterpret_cast<void *>(1),
                                  &data);
    ASSERT_EQ(res, 1);
    ASSERT_TRUE(armd__hash_table_exists(hash_table, 1));
    ASSERT_EQ(armd__hash_table_get_num_entries(hash_table), 1u);
    res = armd__hash_table_get(hash_table, 1, &data);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data, reinterpret_cast<void *>(1));
    res = armd__hash_table_upsert(hash_table, 1, reinterpret_cast<void *>(2),
                                  &data);
    ASSERT_EQ(res, 0);

    res = armd__hash_table_get(hash_table, 1, &data);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data, reinterpret_cast<void *>(2));

    res = armd__hash_table_remove(hash_table, 1);
    ASSERT_EQ(res, 0);
    ASSERT_FALSE(armd__hash_table_exists(hash_table, 1));
    ASSERT_EQ(armd__hash_table_get_num_entries(hash_table), 0u);
}

TEST_F(HashTableTest, CheckUpdate) {
    int res;
    void *data;
    ASSERT_EQ(armd__hash_table_get_num_entries(hash_table), 0u);

    res = armd__hash_table_update(hash_table, 1, reinterpret_cast<void *>(100),
                                  &data);
    ASSERT_NE(res, 0);

    res = armd__hash_table_insert(hash_table, 1, reinterpret_cast<void *>(1));
    ASSERT_EQ(res, 0);
    ASSERT_TRUE(armd__hash_table_exists(hash_table, 1));
    ASSERT_EQ(armd__hash_table_get_num_entries(hash_table), 1u);
    res = armd__hash_table_get(hash_table, 1, &data);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data, reinterpret_cast<void *>(1));
    res = armd__hash_table_update(hash_table, 1, reinterpret_cast<void *>(2),
                                  &data);
    ASSERT_EQ(res, 0);

    res = armd__hash_table_get(hash_table, 1, &data);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data, reinterpret_cast<void *>(2));

    res = armd__hash_table_remove(hash_table, 1);
    ASSERT_EQ(res, 0);
    ASSERT_FALSE(armd__hash_table_exists(hash_table, 1));
    ASSERT_EQ(armd__hash_table_get_num_entries(hash_table), 0u);
}

TEST_F(HashTableTest, CheckExpand) {
    int res;
    void *data;
    ASSERT_EQ(armd__hash_table_get_num_entries(hash_table), 0u);

    res = armd__hash_table_insert(hash_table, 1, reinterpret_cast<void *>(1));
    ASSERT_EQ(res, 0);
    res = armd__hash_table_insert(hash_table, 2, reinterpret_cast<void *>(2));
    ASSERT_EQ(res, 0);
    res = armd__hash_table_insert(hash_table, 3, reinterpret_cast<void *>(3));
    ASSERT_EQ(res, 0);
    res = armd__hash_table_insert(hash_table, 4, reinterpret_cast<void *>(4));
    ASSERT_EQ(res, 0);
    res = armd__hash_table_insert(hash_table, 5, reinterpret_cast<void *>(5));
    ASSERT_EQ(res, 0);

    ASSERT_GT(hash_table->table_size, 4u);

    res = armd__hash_table_get(hash_table, 1, &data);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data, reinterpret_cast<void *>(1));
    res = armd__hash_table_get(hash_table, 2, &data);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data, reinterpret_cast<void *>(2));
    res = armd__hash_table_get(hash_table, 3, &data);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data, reinterpret_cast<void *>(3));
    res = armd__hash_table_get(hash_table, 4, &data);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data, reinterpret_cast<void *>(4));
    res = armd__hash_table_get(hash_table, 5, &data);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data, reinterpret_cast<void *>(5));

    res = armd__hash_table_remove(hash_table, 1);
    ASSERT_EQ(res, 0);
    res = armd__hash_table_remove(hash_table, 2);
    ASSERT_EQ(res, 0);
    res = armd__hash_table_remove(hash_table, 3);
    ASSERT_EQ(res, 0);
    res = armd__hash_table_remove(hash_table, 4);
    ASSERT_EQ(res, 0);
    res = armd__hash_table_remove(hash_table, 5);
    ASSERT_EQ(res, 0);
}

} // namespace
