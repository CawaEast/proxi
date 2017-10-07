#ifndef PROXY_SERVER_TIMER_FD_H
#define PROXY_SERVER_TIMER_FD_H


#include "../util/file_descriptor.h"

struct timer_fd : file_descriptor {
    enum clock_mode {
        REALTIME, MONOTONIC
    };
    enum fd_mode {
        NONBLOCK, CLOEXEC, SIMPLE
    };

    timer_fd();

    timer_fd(clock_mode cmode, fd_mode mode);

    timer_fd(clock_mode cmode, std::initializer_list<fd_mode> mode);

    // Set interval for ticking
    void set_interval(long interval_sec, long start_after_sec) const;

private:
    int value_of(std::initializer_list<fd_mode> mode);
};


#endif //PROXY_SERVER_TIMER_FD_H
