#pragma once
#include "conn.h"
#include <vector>

const size_t K_MAX_MSG {4096};
typedef std::vector<u_int8_t> Buffer;

enum {
    ERR_UNKNOWN = 1,    // unknown command
    ERR_TOO_BIG = 2,    // response too big
} errs;

int32_t read_full(int fd, char* buff, size_t n);
int32_t write_all(int fd, char* buff, size_t n);
int32_t one_request(int connfd);
int32_t query(int fd, const char* text);
void msg(const char msg[]);

// Append functions by supported type
void buffer_append_nil(Buffer &buff);
void buffer_append_string(Buffer &buff, const char *c, size_t size);
void buffer_append_int(Buffer &buff, int64_t val);
void buffer_append_array(Buffer &buff, u_int32_t len);
void buffer_append_error(Buffer &buff, u_int32_t code, const std::string& msg);

void buffer_append_8(Buffer &buff, u_int8_t data);
template <typename T>
//template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>, T>>
void buffer_append_32(Buffer &buff, T data);
//template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>, T>>
template <typename T>
void buffer_append_64(Buffer &buff, T data);

void buffer_append(Buffer &buff, const u_int8_t* data, size_t len);
void buffer_consume(Buffer &buff, size_t len);


void do_request(std::vector<std::string> &cmd, Conn *conn);

void handle_read(Conn* conn);
void handle_write(Conn* conn);

bool try_one_request(Conn *conn);

void set_header_position(Buffer &buff, size_t* header_pos);
size_t get_response_size(Buffer &buff, size_t header);
void check_response(Buffer &buff, size_t header);

