#pragma once

#include <vector>

enum {
    TAG_NIL = 0,    // nil
    TAG_ERR = 1,    // error code + msg
    TAG_STR = 2,    // string
    TAG_INT = 3,    // int64
    TAG_DBL = 4,    // double
    TAG_ARR = 5,    // array
} tag_dict;

struct Conn {
    // File descriptor (-1 because a no connection by default)
    int fd {-1};

    // Checks if the read buffer needs to read in this loop
    bool want_read {false};
    // Checks if the write buffer needs to written to in this loop
    bool want_write {false};
    // Checks if the connection is to be closed in this loop
    bool want_close {false};

    // Read buffer for each loop
    std::vector<uint8_t> incoming;
    // Write buffer for each loop
    std::vector<uint8_t> outgoing;
};

struct B {
    // Pointer to beginning of header
    uint8_t* buffer_begin;
    // Pointer to end of header
    uint8_t* buffer_end;
    // Pointer to beginning of data
    uint8_t* data_begin;
    // Pointer to end of data
    uint8_t* data_end;
};

struct Response {
    std::vector<uint8_t> &data;
    uint32_t status {0};
};