#pragma once

#include <functional>
#include <string>
#include "conn.h"

const size_t MAX_LOAD_FACTOR {8};
const size_t REHASH_AMOUNT {128};

#define container_of(ptr, T, member) \
    ((T *)( (char *)ptr - offsetof(T, member) ))

// Node
struct Node {
    // Pointer to next node
    Node* next {nullptr};
    // Raw hash value - Masked to get bucket info
    uint64_t hash_value {0};
};

// Hash Table
struct HashTable {
    // Pointer to Node* calloc
    Node **table {nullptr};
    // Mask to act as raw mod function
    size_t mask {0};
    // Current size of hash table
    size_t size {0};
};

// Progressive Hash Map
struct HashMap {
    // Table used for lookup
    HashTable newer;
    // Older table used for progressive rehashing
    HashTable older;
    size_t migrate_pos {0};
};

template <class T>
struct Data {
    T data;
    Node node;
};

static struct g_data{
    HashMap db;
} g_data;

struct Entry {
    Node node;
    std::string key;
    std::string val;
};

// Hash Table functions
void createHashTable(HashTable* h_table, size_t n);
void insertIntoHashTable(HashTable* h_table, Node* node);
Node** FindInHashTable(HashTable* h_table, Node* key, std::function<bool(Node*, Node*)> eq);
Node* removeFromHashTable(HashTable* h_table, Node** prev);

// Hash Map functions
Node* findInHashMap(HashMap* h_map, Node* key, std::function<bool(Node*, Node*)> eq);
Node* removeFromHashMap(HashMap* h_map, Node* key, std::function<bool(Node*, Node*)> eq);
void insertIntoHashMap(HashMap* h_map, Node* key);
void triggerHashMapResize(HashMap* h_map);
void resizeHashMap(HashMap* h_map);

// Checks for equal node
bool checkNodeEquality(Node* lhs, Node* rhs);

// Interfaces with server functions
void get(std::vector<std::string> &cmd, Response &resp, const std::string** val);
void set(std::vector<std::string> &cmd, Response &resp);
void del(std::vector<std::string> &cmd, Response &resp);

uint64_t str_hash(const uint8_t *data, size_t len);