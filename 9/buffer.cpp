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
#include <vector>
#include <map>
#include <errno.h>
#include <fcntl.h>
#include <type_traits>

#include "buffer.h"
#include "time.h"
#include "conn.h"
#include "map.h"

//std::map<std::string, std::string> g_data;

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

// BUFFER MANIPULATION FUNCTIONS

// Adds the data to buffer
void buffer_append(Buffer &buff, const u_int8_t* data, size_t len) {
    std::cout << buff.size() << " " << data << " " << len << std::endl;
    buff.insert(buff.end(), data, data + len);
    
}

// Removes the data from the buffer
void buffer_consume(Buffer &buff, size_t len) {
    buff.erase(buff.begin(), buff.begin() + len);
}

// Appends u_int8_t data directly to the buffer
void buffer_append_8(Buffer &buff, u_int8_t data) {
    buff.push_back(data);
}

// Casts 32 bit numeric data to u_int8_t and calls buffer_append
template <typename T>
void buffer_append_32(Buffer &buff, T data) {
    std::cout << "Appending 32-bit type" << '\n';
    buffer_append(buff, reinterpret_cast<const uint8_t*>(&data), 4);
}

// Casts 64 bit numeric data to u_int8_t and calls buffer_append
template <typename T>
void buffer_append_64(Buffer &buff, T data) {
    std::cout << "Appending 64-bit type" << '\n';
    buffer_append(buff, reinterpret_cast<const uint8_t*>(&data), 8);
}

void buffer_append_nil(Buffer &buff) {
    std::cout << "Appending nil" << '\n';
    buffer_append_8(buff, TAG_NIL);
}

void buffer_append_string(Buffer &buff, const char *c, size_t size) {
    std::cout << "Appending string" << '\n';
    buffer_append_8(buff, TAG_STR);
    buffer_append_32(buff, static_cast<u_int32_t>(size));
    buffer_append(buff, reinterpret_cast<const u_int8_t*>(c), size);
}

void buffer_append_int(Buffer &buff, int64_t val) {
    std::cout << "Appending int" << '\n';
    buffer_append_8(buff, TAG_INT);
}

void buffer_append_array(Buffer &buff, u_int32_t len) {
    std::cout << "Appending array" << '\n';
    buffer_append_8(buff, TAG_ARR);
    buffer_append_32(buff, len);
}

void buffer_append_error(Buffer &buff, u_int32_t code, const std::string& msg) {
    std::cout << "Appending error" << '\n';
    buffer_append_8(buff, TAG_ERR);
    buffer_append_32(buff, code);
    buffer_append_32(buff, static_cast<u_int32_t>(msg.size()));
    buffer_append(buff, reinterpret_cast<const u_int8_t*>(msg.data()), msg.size());
}


void handle_read(Conn* conn) {
    u_int8_t buff[64 * 1024];
    ssize_t rv { read(conn->fd, buff, sizeof(buff)) }; // Returns number of bytes read

    if (rv <= 0) {
        conn->want_close = true;
        return;
    }


    buffer_append(conn->incoming, buff, static_cast<ssize_t>(rv));

    //std::cout << getTime() << " - Client: " << &conn->incoming[4] << std::endl;

    std::cout << "Trying request" << std::flush;

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

// Reads the count of strings/Size of payload
bool read_u32(const uint8_t* &cur, const uint8_t* &end, uint32_t &out) {
    // If less than 4 bytes left, this is not a valid message
    if (cur + 4 > end) { return false; };
    // Copies the size of the incoming message if len passed in or number of arguments if nstr passed in
    std::memcpy(&out, cur, 4);
    // Advances data 4 bytes to start of payload
    cur += 4;
    return true;
}

bool read_str(const uint8_t* &cur, const uint8_t* &end, uint32_t &n, std::string &out) {
    // If the length in the header is longer than the message
    if (cur + n > end) { return false; }
    // Assigns the payload to n
    out.assign(cur, cur + n);
    // Advances cur by n places
    cur += n;
    return true;

}

uint32_t parse_req(const uint8_t* data, uint32_t len, std::vector<std::string> &out) {
    // End of the message
    const uint8_t* end {data + len};
    // Number of strings in request
    uint32_t nstr {0};

    // Gets number of messages
    if (!read_u32(data, end, nstr)) { return -1; };
    if (nstr > K_MAX_MSG) { return -1; };

    while (out.size() < nstr) {
        uint32_t len {0};
        // Gets length of string in message
        if (!read_u32(data, end, len)) { return -1; };

        out.push_back(std::string());
        // Gets string in message and appends it to last element of out
        if (!read_str(data, end, len, out.back())) { return -1; };
    }

    if (data != end) { return -1; };

    return 0;

}

void set_header_position(Buffer &buff, size_t* header_pos) {
    *header_pos = buff.size();
    buffer_append_32(buff, 0);
}

size_t get_response_size(Buffer &buff, size_t header) {
    return buff.size() - header - 4;
}

void check_response(Buffer &buff, size_t header) {
    size_t msg_size {get_response_size(buff, header)};
    if (msg_size > K_MAX_MSG) {
        buff.resize(header + 4);
        buffer_append_error(buff, ERR_TOO_BIG, "Response too large");
        msg_size = get_response_size(buff, header);
    }

    u_int32_t msg_len {static_cast<u_int32_t>(msg_size)};
    std::memcpy(&buff[0], &msg_len, 4);
}

void do_request(std::vector<std::string> &cmd, Conn *conn) {
    int status {0};

    if (cmd.size() == 2 && cmd.at(0) == "get") {
        get(cmd, conn);
    } else if (cmd.size() == 3 && cmd.at(0) == "set") {
        set(cmd, conn);
    } else if (cmd.size() == 2 && cmd.at(0) == "del") {
        del(cmd, conn);
    } else if (cmd.size() == 1 && cmd.at(0) == "keys") {
        keys(cmd, conn);
    } else {
        buffer_append_error(conn->outgoing, 4, "unknown command");
    }
}

void make_response(const Response &resp, std::vector<uint8_t> &out) {
    uint32_t resp_len = 4 + (uint32_t)resp.data.size();
    buffer_append(out, (const uint8_t *)&resp_len, 4);
    buffer_append(out, (const uint8_t *)&resp.status, 4);
    buffer_append(out, resp.data.data(), resp.data.size());
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
    
    // Request is the message on the read buffer
    const uint8_t* request {&conn->incoming[4]};

    std::vector<std::string> cmd;

    if (parse_req(request, request_query_len, cmd) < 0) {
        conn->want_close = true;
        return false;
    }

    size_t header_position {0};
    set_header_position(conn->outgoing, &header_position);
    do_request(cmd, conn);
    check_response(conn->outgoing, header_position);

    // Removes the query from the read buffer
    buffer_consume(conn->incoming, request_query_len + 4);
    

    return true;

}




