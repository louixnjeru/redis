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

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    //std::cout << fd << std::endl;

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);

    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));

    //std::cout << fd << std::endl;

    char write_buffer[] {"Hello"};

    write(fd, write_buffer, sizeof(write_buffer));

    //std::cout << "Written\n";

    char read_buffer[64] {};

    ssize_t n = read(fd, read_buffer, sizeof(read_buffer) - 1);
    if (n < 0) {
        std::cout << "Read error\n";
        return 1;
    }

    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm* local_tm = std::localtime(&now_time_t);

    std::cout << "Server says: " << read_buffer << " @ " << std::put_time(local_tm, "%T.%N") << std::endl;

    return 0;
}