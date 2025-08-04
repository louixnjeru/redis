#pragma once

#include <vector>

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

struct Buffer {
    // Pointer to beginning of header
    uint8_t* buffer_begin;
    // Pointer to end of header
    uint8_t* buffer_end;
    // Pointer to beginning of data
    uint8_t* data_begin;
    // Pointer to end of data
    uint8_t* data_end;
};