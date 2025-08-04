#pragma once

#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <cstring>

const size_t K_MAX_MSG {4096};

int32_t read_full(int fd, char* buff, size_t n);
int32_t write_all(int fd, char* buff, size_t n);
int32_t one_request(int connfd);
int32_t query(int fd, const char* text);
void msg(const char msg[]);
