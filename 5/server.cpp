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

#include "main.h"

void die(const char *msg) {
    std::cout << msg << std::endl;
    abort();
}



int main() {
    std::cout << "Starting" << std::endl;
    int rv;
    int fd { socket(AF_INET, SOCK_STREAM, 0) };
    //std::cout << fd << std::endl;
    int val {1};
    rv = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    //std::cout << rv << " " << fd << std::endl;

    // Sets up listening socket on 127.0.0.1:1234
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(0);

    // Binds the address to the socket
    rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));

    if (rv) {
        msg("Bind Error");
    }

    /*
    Sets up the socket as listening socket with SOMAXCONN maximum connections
    SOMAXCONN is 128 on this compiler
    */
    rv = listen(fd, SOMAXCONN);

    if (rv) {
        msg("Listen Error");
    }

    // Vector map for fd -> connection objects
    std::vector<Conn*> fd2Conn;
    std::vector<struct pollfd> poll_args;

    // Sets the listening socket to be non-blocking
    fd_set_nb(fd);

    std::cout << "Listening" << std::endl;
    while (true) {
        std::cout << "\n";
        // Clears the client connections of the last loop
        poll_args.clear();
        // Places the listening socket at the front of the list
        pollfd pfd{fd, POLLIN, 0};
        poll_args.push_back(pfd);

        // Gets the list of active connections and creating structs to monitor them
        for (Conn* conn : fd2Conn) {
            if (!conn) {
                continue;
            }

            pollfd poll_fd{conn->fd, POLLERR, 0};

            // If connection wants to read, bitmask POLLIN flag
            if (conn->want_read) {
                poll_fd.events |= POLLIN;
            }

            // If connection wants to write, bitmask POLLOUT flag
            if (conn->want_write) {
                poll_fd.events |= POLLOUT;
            }

            poll_args.push_back(poll_fd);
        }

        // Polls for readiness
        rv = poll(poll_args.data(), static_cast<nfds_t>(poll_args.size()), -1);

        if (rv < 0 && errno == EINTR) {
            continue;
        }

        if (rv < 0) {
            die("poll error");
        }

        // Maps the listening socket
        if(poll_args[0].revents) {
            if (Conn* conn = handle_accept(fd)) {
                // Resizes the socket map if necessary
                if (fd2Conn.size() <= static_cast<size_t>(conn->fd)) {
                    fd2Conn.resize(conn->fd + 1);
                }
                fd2Conn[conn->fd] = conn;
            }
        }

        // Maps the connection sockets
        for (int i{1}; i < poll_args.size(); ++i) {
            short ready {poll_args[i].revents};
            Conn* conn = fd2Conn[poll_args[i].fd];

            if (ready & POLLIN) {
                handle_read(conn);
            }

            if (ready & POLLOUT) {
                handle_write(conn);
            }

            if ((ready & POLLERR) || conn->want_close) {
                static_cast<void>(close(conn->fd));
                fd2Conn[conn->fd] = NULL;
                delete conn;
            }
        }
        /*
        std::cout << "\n";
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);

        // Prints client IP
        char client_info[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr, client_info, addrlen);
        std::cout << "Accepted request from " << client_info << std::endl;


        if (connfd < 0) {
            std::cout << "Conn error\n" << std::endl;
            continue;
        }

        while (true) {
            int32_t err { one_request(connfd) };
            if (err) {
                break;
            }
        }
        close(connfd);
        */
    }
    


    return 0;
}


