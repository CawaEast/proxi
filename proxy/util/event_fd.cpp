#include "event_fd.h"
#include "annotated_exception.h"
#include <sys/eventfd.h>

event_fd::event_fd() : file_descriptor() {}

event_fd::event_fd(unsigned int initial, fd_mode mode) : event_fd(initial, {mode}) {}

event_fd::event_fd(unsigned int initial, std::initializer_list<fd_mode> mode) {
    if ((fd = eventfd(initial, value_of(mode))) == -1) {
        int err = errno;
        throw annotated_exception("eventfd", err);
    }
}

int event_fd::value_of(std::initializer_list<fd_mode> mode) {
    int res = 0;
    for (auto it = mode.begin(); it != mode.end(); it++) {
        switch (*it) {
            case NONBLOCK:
                res |= EFD_NONBLOCK;
                break;
            case CLOEXEC:
                res |= EFD_CLOEXEC;
                break;
            case SEMAPHORE:
                res |= EFD_SEMAPHORE;
                break;
            case SIMPLE:
                res |= 0;
                break;
        }
    }
    return res;
}