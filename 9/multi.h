#pragma once

void fd_set_nb(int fd);

struct Conn* handle_accept(int fd);

void handle_read(Conn* conn);
void handle_write(Conn* conn);