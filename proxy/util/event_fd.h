#ifndef PROXY_SERVER_EVENT_FD_H
#define PROXY_SERVER_EVENT_FD_H

#include "file_descriptor.h"

struct event_fd : file_descriptor {
    enum fd_mode {
        NONBLOCK, CLOEXEC, SEMAPHORE, SIMPLE
    };

    event_fd();

    event_fd(unsigned int initial, fd_mode mode);

    event_fd(unsigned int initial, std::initializer_list<fd_mode> mode);

private:
    int value_of(std::initializer_list<fd_mode> mode);
};


#endif //PROXY_SERVER_EVENT_FD_H
