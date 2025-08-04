#pragma once
#include "conn.h"
#include <vector>

const size_t K_MAX_MSG {4096};

int32_t read_full(int fd, char* buff, size_t n);
int32_t write_all(int fd, char* buff, size_t n);
int32_t one_request(int connfd);
int32_t query(int fd, const char* text);
void msg(const char msg[]);

void buffer_append(std::vector<u_int8_t> &buff, const u_int8_t* data, size_t len);
void buffer_consume(std::vector<u_int8_t> &buff, size_t len);

void handle_read(Conn* conn);
void handle_write(Conn* conn);

bool try_one_request(Conn *conn);
