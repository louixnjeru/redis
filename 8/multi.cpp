#include "multi.h"
#include "conn.h"

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
#include <poll.h>
#include <fcntl.h>

// Sets the fd connection to be non-blocking
void fd_set_nb(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

// Creates a new conn struct from the fd connection
struct Conn* handle_accept(int fd) {
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);

    if (connfd < 0) {
        return NULL;
    }

    fd_set_nb(fd);

    Conn* new_conn = new Conn();
    new_conn->fd = connfd;
    new_conn->want_read = true;

    return new_conn;
}