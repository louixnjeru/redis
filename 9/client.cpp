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

#include "main.h"

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    //std::cout << fd << std::endl;

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);

    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));

    int32_t err;
    bool done {false};


    while (!done) {
        /*
        err = query(fd, "hello1");
        if (err) { break; }

        err = query(fd, "hello2");
        if (err) { break; }
        */
        std::vector<std::string> query_list{"hello1", "hello2", "hello3", "hello4", "hello5"};

        for (const auto &q : query_list) {
            err = query(fd, q.data());
            if (err) {std::cout << err << std::endl;}
        }

        done = true;
    }

    close(fd);
    


    return 0;
}