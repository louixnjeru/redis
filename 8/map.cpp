#include "map.h"
#include "buffer.h"

#include <cstdlib>
#include <stdlib.h>
#include <cassert>
#include <functional>
#include <iostream>

void createHashTable(HashTable* h_table, size_t n) {
    assert(n > 0 && (n & (n-1)) == 0);
    h_table->table = reinterpret_cast<Node**>(std::calloc(n, sizeof(Node*)));
    h_table->mask = n-1;
}

void insertIntoHashTable(HashTable* h_table, Node* node) {
    std::cout << "Inserting into hash table" << h_table << std::endl;
    // Gets pos by bitwise masking
    size_t pos = node->hash_value & h_table->mask;
    // Sets next pointer to point to first node in bucket
    node->next = h_table->table[pos];
    // Sets pointer in bucket to point to new node
    h_table->table[pos] = node;
    h_table->size++;
}

Node** FindInHashTable(HashTable* h_table, Node* key, std::function<bool(Node*, Node*)> eq) {
    // If the hash table doesn't exist, return a null pointer
    if (!h_table->table) {
        return nullptr;
    }

    /*
    The hash function that determines the bucket
    AND operation on the hash code and the mask (array size)
    */
    size_t pos {key->hash_value & h_table->mask};

    Node** from {&h_table->table[pos]};

    /*
    Initalises a pointer to node cur
    Sets cur to "dereferenced from" (pointer to this node from previous)
    Iterates by setting from to be a pointer to the cur's next variable, thus Node** (pointer to node pointer)
    */
    for (Node* cur; (cur = *from) != nullptr; from = &cur->next) {
        if (cur->hash_value == key->hash_value && eq(cur, key)) {
            return from;
        }
    }

    return nullptr;

}

Node* removeFromHashTable(HashTable* h_table, Node** prev) {
    Node* node = *prev;
    *prev = node->next;
    h_table->size--;

    return node;

}

Node* findInHashMap(HashMap* h_map, Node* key, std::function<bool(Node*, Node*)> eq) {
    resizeHashMap(h_map);
    // Attempts to find key in newer Hash Table
    Node** from {FindInHashTable(&h_map->newer, key, eq)};
    
    if (!from) {
        from = FindInHashTable(&h_map->older, key, eq);
    }

    return (from ? *from : nullptr);
}

Node* removeFromHashMap(HashMap* h_map, Node* key, std::function<bool(Node*, Node*)> eq) {
    resizeHashMap(h_map);
    Node** from {FindInHashTable(&h_map->newer, key, eq)};
    
    if (from) {
        return removeFromHashTable(&h_map->newer, from);
    }

    from = FindInHashTable(&h_map->older, key, eq);

    if (from) {
        return removeFromHashTable(&h_map->older, from);
    }

    return *from;

}

void insertIntoHashMap(HashMap* h_map, Node* key) {
    if (!h_map->newer.table) {
        createHashTable(&h_map->newer, 4);
    }

    insertIntoHashTable(&h_map->newer, key);

    if (!h_map->older.table) {
        size_t threshold { (h_map->newer.mask + 1) * MAX_LOAD_FACTOR };
        if (h_map->newer.size > threshold) {
            triggerHashMapResize(h_map);
        }
    }

    resizeHashMap(h_map);

    std::cout << "Exiting insert" << std::endl;

}

void triggerHashMapResize(HashMap* h_map) {
    // Moves the current hash map to the older slot
    h_map->older = h_map->newer;
    // Initialised a new hash map double the size of the old one
    createHashTable(&h_map->newer, (h_map->newer.mask + 1) * 2);
    h_map->migrate_pos = 0;
}

void resizeHashMap(HashMap* h_map) {
    size_t rehash_count {0};

    while (rehash_count < REHASH_AMOUNT && h_map->older.size > 0) {
        // Gets first node at migrate pos
        Node** prev { &h_map->older.table[h_map->migrate_pos] };

        // If nothing left in bucket
        if (!*prev) {
            h_map->migrate_pos++;
            continue;
        }

        // Removes head of bucket and places it into newer
        insertIntoHashTable(&h_map->newer, removeFromHashTable(&h_map->older, prev));
        rehash_count++;
    }

    // Frees the space
    if (h_map->older.size == 0 && h_map->older.table) {
        std::free(h_map->older.table);
        h_map->older = HashTable{};
    }
}

bool checkNodeEquality(Node* lhs, Node* rhs) {
    struct Entry* l {container_of(lhs, struct Entry, node)};
    struct Entry* r {container_of(rhs, struct Entry, node)};

    return l->key == r->key;
}

uint64_t str_hash(const uint8_t *data, size_t len) {
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++) {
        h = (h + data[i]) * 0x01000193;
    }
    return h;
}

void get(std::vector<std::string> &cmd, Response &resp, const std::string** val) {
    Entry key;

    key.key.swap(cmd[1]);
    key.node.hash_value = str_hash(reinterpret_cast<uint8_t*>(key.key.data()), key.key.size());
    
    Node* node {findInHashMap(&g_data.db, &key.node, checkNodeEquality)};

    if (!node) {
        resp.status = 2;
        return;
    }

    *val = &container_of(node, Entry, node)->val;

    std::cout << *val << " " << **val << std::endl;

}

void set(std::vector<std::string> &cmd, Response &resp) {
    Entry key;

    key.key.swap(cmd[1]);
    key.node.hash_value = str_hash(reinterpret_cast<uint8_t*>(key.key.data()), key.key.size());
    
    Node* node {findInHashMap(&g_data.db, &key.node, checkNodeEquality)};

    if (node) {
        container_of(node, Entry, node)->val.swap(cmd[2]);
    } else {
        std::cout << "Creating new entry" << std::endl;
        Entry* ent = new Entry();
        ent->key.swap(key.key);
        ent->node.hash_value = key.node.hash_value;
        ent->val.swap(cmd[2]);
        insertIntoHashMap(&g_data.db, &ent->node);
    }



}

void del(std::vector<std::string> &cmd, Response &resp) {
    Entry key;

    key.key.swap(cmd[1]);
    key.node.hash_value = str_hash(reinterpret_cast<uint8_t*>(key.key.data()), key.key.size());

    Node* node {removeFromHashMap(&g_data.db, &key.node, checkNodeEquality)};

    if (node) {
        delete container_of(node, Entry, node);
    }

}