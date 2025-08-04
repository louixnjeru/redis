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

#include "buffer.h"
#include "time.h"

void msg(const char msg[]) {
    std::cout << msg << std::endl;
}

int32_t read_full(int fd, char* buff, size_t n) {
    // While there are bytes left to read
    while (n > 0) {
        ssize_t rv { read(fd, buff, n) }; // Returns number of bytes read
        if (rv <= 0 && errno != EINTR) {
            return -1;
        } // rv < 0 if error, rv = 0 if nothing read

        assert(static_cast<size_t>(rv) <= n); // Terminating error if bytes read is greater than remaining bytes, as determined by prefix (n)
        n -= static_cast<size_t>(rv); // Number of bytes left to read subtracted by number of bytes read
        buff += rv; // buff pointer advanced by bytes read
    }

    return 0;
    
}

int32_t write_all(int fd, char* buff, size_t n) {
    while (n > 0) {
        ssize_t rv { write(fd, buff, n) };
        if (rv <= 0) {
            return -1;
        }

        assert(static_cast<size_t>(rv) <= n);
        n -= static_cast<size_t>(rv);
        buff += rv;
    }

    return 0;
}

int32_t one_request(int connfd) {
    char rbuff[4 + K_MAX_MSG];
    errno = 0;

    int32_t err { read_full(connfd, rbuff, 4) };
    if (err) {
        msg((errno == 0? "EOF" : "read() error"));
        return err;
    }

    uint32_t len {0};

    std::memcpy(&len, rbuff, 4);
    if (len > K_MAX_MSG) {
        msg("Too long");
    }

    err = read_full(connfd, &rbuff[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    //printf("server says: %.*s\n", len, &rbuff[4]);
    std::cout << getTime() << " - Client: " << &rbuff[4] << std::endl;

    const char reply[] {"world"};

    char wbuff[4 + sizeof(reply) + 1];
    len = static_cast<u_int32_t>(strlen(reply));

    std::memcpy(wbuff, &len, 4);
    std::memcpy(&wbuff[4], reply, len);

    return write_all(connfd, wbuff, 4+len);
}

int32_t query(int fd, const char* text) {
    uint32_t len {static_cast<u_int32_t>(strlen(text))};
    if (len > K_MAX_MSG) {
        msg("Too long");
        return -1;
    }

    char wbuff[4 + K_MAX_MSG + 1];
    std::memcpy(wbuff, &len, 4);
    std::memcpy(&wbuff[4], text, len);

    if (u_int32_t err = write_all(fd, wbuff, 4+len)) {
        return err;
    }

    char rbuff[4 + K_MAX_MSG + 1];
    errno = 0;

    int32_t err { read_full(fd, rbuff, 4) };
    if (err) {
        msg((errno == 0? "EOF" : "read() error"));
        return err;
    }

    std::memcpy(&len, rbuff, 4);
    if (len > K_MAX_MSG) {
        msg("Too long");
    }

    err = read_full(fd, &rbuff[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    //printf("server says: %.*s\n", len, &rbuff[4]);
    std::cout << getTime() << " - Server: " << &rbuff[4] << std::endl;

    return 0;

}