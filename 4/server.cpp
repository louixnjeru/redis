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

#include "main.h"

void die(const char *msg) {
    std::cout << msg << std::endl;
    abort();
}

int main() {
    std::cout << "Starting" << std::endl;
    int rv;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    //std::cout << fd << std::endl;
    int val {1};
    rv = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    //std::cout << rv << " " << fd << std::endl;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(0);

    rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));

    if (rv) {
        msg("Bind Error");
    }

    rv = listen(fd, SOMAXCONN);

    if (rv) {
        msg("Listen Error");
    }

    std::cout << "Listening" << std::endl;
    while (true) {
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
    }
    


    return 0;
}


