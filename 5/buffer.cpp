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
#include <cassert>

#include "buffer.h"
#include "time.h"
#include "conn.h"

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

// Adds the data to buffer
void buffer_append(std::vector<u_int8_t> &buff, const u_int8_t* data, size_t len) {
    buff.insert(buff.end(), data, data + len);
}

// Removes the data from the buffer
void buffer_consume(std::vector<u_int8_t> &buff, size_t len) {
    buff.erase(buff.begin(), buff.begin() + len);
}

void handle_read(Conn* conn) {
    u_int8_t buff[64 * 1024];
    ssize_t rv { read(conn->fd, buff, sizeof(buff)) }; // Returns number of bytes read

    if (rv <= 0) {
        conn->want_close = true;
        return;
    }


    buffer_append(conn->incoming, buff, static_cast<ssize_t>(rv));

    std::cout << getTime() << " - Client: " << &conn->incoming[4] << std::endl;

    while(try_one_request(conn)) {};

    // If all data read, switch to write state
    if (conn->outgoing.size() > 0) {
        conn->want_read = false;
        conn->want_write = true;
        return handle_write(conn);
    }

}

void handle_write(Conn* conn) {
    assert(conn->outgoing.size() > 0);
    ssize_t rv { write(conn->fd, conn->outgoing.data(), conn->outgoing.size())};

    if (rv < 0 && errno == EAGAIN) {
        return;
    }

    if (rv < 0) {
        conn->want_close = true;
        return;
    }

    buffer_consume(conn->outgoing, rv);

    // If all data written, switch to read state
    if (conn->outgoing.size() == 0) {
        conn->want_write = false;
        conn->want_read = true;
    }

}

bool try_one_request(Conn *conn) {
    if (conn->incoming.size() < 4) {
        return false;
    }

    uint32_t request_query_len{0};

    // Copys the header
    std::memcpy(&request_query_len, conn->incoming.data(), 4);

    // If there is a protocol error
    if (request_query_len > K_MAX_MSG) {
        // Connection will be closed
        conn->want_close = true;
        return false;
    }

    if (4 + request_query_len > conn->incoming.size()) {
        return false;
    }

    /*

    // Request is the message on the read buffer
    const char* request {"world"};
    size_t response_query_len { strlen(request) };
    
    // Puts header on write buffer (length of message)
    buffer_append(conn->outgoing, (const uint8_t*)&response_query_len, 4);
    //std::cout << (const uint8_t*)request << len << std::endl;
    //std::cout << conn->incoming.data() << std::endl;
    // Puts server response on write buffer
    buffer_append(conn->outgoing, (const uint8_t*)request, response_query_len);

    // Removes the query from the read buffer
    buffer_consume(conn->incoming, request_query_len + 4);

    */
    
    // Request is the message on the read buffer
    const uint8_t* request {&conn->incoming[4]};
    // Puts header on write buffer (length of message)
    buffer_append(conn->outgoing, (const uint8_t*)&request_query_len, 4);
    // Puts server response on write buffer
    buffer_append(conn->outgoing, request, request_query_len);

    // Removes the query from the read buffer
    buffer_consume(conn->incoming, request_query_len + 4);
    

    return true;

}