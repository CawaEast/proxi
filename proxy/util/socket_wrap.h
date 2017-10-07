#ifndef PROXY_SERVER_SOCKET_WRAP_H
#define PROXY_SERVER_SOCKET_WRAP_H

#include "file_descriptor.h"

struct adress_t {
    uint32_t ip;
    uint16_t port;

};

struct socket_wrap : file_descriptor {
    enum socket_mode {
        NONBLOCK, CLOEXEC, SIMPLE
    };

    socket_wrap(socket_mode mode);

    socket_wrap(std::initializer_list<socket_mode> mode);

    socket_wrap(socket_wrap &&other);

    // Accept other socket
    socket_wrap accept(socket_mode mode) const;

    socket_wrap accept(std::initializer_list<socket_mode> mode) const;

    // Bing to port
    void bind(uint16_t port) const;

    // Connect to adress_t
    void connect(adress_t address) const;

    // Listen to incoming connections
    void listen(int queue_size) const;

    // Method that calls getsockopt
    void get_option(int name, void *res, socklen_t *res_len) const;

    friend std::string to_string(socket_wrap &wrap);

protected:
    socket_wrap();

    explicit socket_wrap(int fd);

private:
    int value_of(std::initializer_list<socket_mode> modes) const;
};


void swap(adress_t &first, adress_t &second);

std::string to_string(adress_t const &ep);


#endif //PROXY_SERVER_SOCKET_WRAP_H
