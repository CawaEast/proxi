#ifndef PROXY_SERVER_SIGNAL_FD_H
#define PROXY_SERVER_SIGNAL_FD_H

#include "file_descriptor.h"

struct signal_fd : file_descriptor {
    enum fd_mode {
        NONBLOCK, CLOEXEC, SIMPLE
    };

    signal_fd();

    signal_fd(int hande, fd_mode mode);

    signal_fd(std::initializer_list<int> handle, std::initializer_list<fd_mode> mode);

private:
    int value_of(std::initializer_list<fd_mode> mode);
};


#endif //PROXY_SERVER_SIGNAL_FD_H
