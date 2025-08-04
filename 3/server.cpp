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

void die(const char *msg) {
    std::cout << msg << std::endl;
    abort();
}

void do_something(int connfd) {
    //std::cout << "Doing\n";
    char read_buffer[64] {};

    ssize_t n = read(connfd, read_buffer, sizeof(read_buffer) - 1);
    if (n < 0) {
        std::cout << "Nothing\n";
        return;
    }

    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm* local_tm = std::localtime(&now_time_t);

    std::cout << "Client says: " << read_buffer << " @ " << std::put_time(local_tm, "%T.%N") << std::endl;

    char write_buffer[] {"world"};

    write(connfd, write_buffer, sizeof(write_buffer));
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
        std::cout << "Bind Error" << std::endl;
    }

    rv = listen(fd, SOMAXCONN);

    if (rv) {
        std::cout << "Listen Error" << std::endl;
    }

    std::cout << "Listening" << std::endl;
    while (true) {
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
        char client_info[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr, client_info, addrlen);
        std::cout << client_info << std::endl;
        if (connfd < 0) {
            std::cout << "Conn error\n" << std::endl;
            continue;
        }
        //std::cout << "*\n" << std::endl;

        do_something(connfd);
        close(connfd);
    }
    


    return 0;
}


