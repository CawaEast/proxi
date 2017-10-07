#include "socket_wrap.h"
#include "annotated_exception.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>


socket_wrap::socket_wrap() :
        file_descriptor() {
}

socket_wrap::socket_wrap(int fd) :
        file_descriptor(fd) {
}

socket_wrap::socket_wrap(socket_mode mode) : socket_wrap({mode}) {}

socket_wrap::socket_wrap(std::initializer_list<socket_mode> mode) :
        file_descriptor() {
    int type = SOCK_STREAM | value_of(mode);

    fd = socket(AF_INET, type, 0);

    if (fd == -1) {
        int err = errno;
        throw annotated_exception("socket", err);
    }
}

socket_wrap::socket_wrap(socket_wrap &&other) {
    swap(*this, other);
}

int socket_wrap::value_of(std::initializer_list<socket_mode> modes) const {
    int mode = 0;
    for (auto it = modes.begin(); it != modes.end(); it++) {
        switch (*it) {
            case SIMPLE:
                mode |= 0;
                break;
            case NONBLOCK:
                mode |= O_NONBLOCK;
                break;
            case CLOEXEC:
                mode |= O_CLOEXEC;
                break;
            default:
                mode |= 0;
                break;
        }
    }
    return mode;
}

socket_wrap socket_wrap::accept(socket_mode mode) const {
    int new_fd = ::accept4(fd, 0, 0, value_of({mode}));
    if (new_fd == -1) {
        int err = errno;
        throw annotated_exception("accept", err);
    }
    return socket_wrap(new_fd);
}

socket_wrap socket_wrap::accept(std::initializer_list<socket_mode> mode) const {

    int new_fd = ::accept4(fd, 0, 0, value_of(mode));
    if (new_fd == -1) {
        int err = errno;
        throw annotated_exception("accept", err);
    }
    return socket_wrap(new_fd);
}

void socket_wrap::bind(uint16_t port) const {
    sockaddr_in addr = {};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr))) {
        int err = errno;
        throw annotated_exception("bind", err);
    }
}

void socket_wrap::connect(adress_t address) const {
    sockaddr_in addr;
    memset(&addr, 0, sizeof addr);

    addr.sin_family = AF_INET;
    addr.sin_port = address.port;
    addr.sin_addr.s_addr = address.ip;

    if (::connect(fd, (struct sockaddr *) (&addr), sizeof(addr))) {
        int err = errno;
        throw annotated_exception("connect", err);
    }
}

void socket_wrap::listen(int max_queue_size) const {
    if (max_queue_size == -1) {
        max_queue_size = SOMAXCONN;
    }

    if (::listen(fd, max_queue_size)) {
        int err = errno;
        throw annotated_exception("listen", err);
    }
}

void socket_wrap::get_option(int name, void *res, socklen_t *res_len) const {
    if (getsockopt(fd, SOL_SOCKET, name, res, res_len) < 0) {
        int err = errno;
        throw annotated_exception("get_option", err);
    }
}


std::string to_string(socket_wrap &wrap) {
    return "socket " + std::to_string(wrap.get());
}

void swap(adress_t &first, adress_t &second) {
    std::swap(first.ip, second.ip);
    std::swap(first.port, second.port);
}

std::string to_string(adress_t const &ep) {
    using std::to_string;
    return to_string(ep.ip) + ":" + to_string(ep.port);
}

