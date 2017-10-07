//
// Created by cawa on 10/6/17.
//

#include "signal_fd.h"
#include "annotated_exception.h"
#include <sys/signalfd.h>
#include "signal.h"

signal_fd::signal_fd() : file_descriptor() {}

signal_fd::signal_fd(int handle, fd_mode mode) : signal_fd({handle}, {mode}) {
}

signal_fd::signal_fd(std::initializer_list<int> handle, std::initializer_list<fd_mode> mode) {
    sigset_t mask;
    sigemptyset(&mask);
    for (auto it = handle.begin(); it != handle.end(); it++) {
        sigaddset(&mask, *it);
    }
    sigprocmask(SIG_BLOCK, &mask, NULL);

    if ((fd = signalfd(-1, &mask, value_of(mode))) == -1) {
        int err = errno;
        throw annotated_exception("signalfd", err);
    }
}

int signal_fd::value_of(std::initializer_list<fd_mode> mode) {
    int res = 0;
    for (auto it = mode.begin(); it != mode.end(); it++) {
        switch (*it) {
            case NONBLOCK:
                res |= SFD_NONBLOCK;
                break;
            case CLOEXEC:
                res |= SFD_CLOEXEC;
                break;
            case SIMPLE:
                res |= 0;
                break;
        }
    }
    return res;
}