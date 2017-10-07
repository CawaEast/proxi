#include "timer_fd.h"
#include "../util/annotated_exception.h"
#include <sys/timerfd.h>

timer_fd::timer_fd() : file_descriptor() {

}

timer_fd::timer_fd(clock_mode cmode, fd_mode mode) : timer_fd(cmode, {mode}) {
}

timer_fd::timer_fd(clock_mode cmode, std::initializer_list<fd_mode> mode) {
    int clock_mode = (cmode == MONOTONIC) ? CLOCK_MONOTONIC : CLOCK_REALTIME;
    if ((fd = timerfd_create(clock_mode, value_of(mode))) == -1) {
        int err = errno;
        throw annotated_exception("timerfd", err);
    }
}

void timer_fd::set_interval(long interval_sec, long start_after_sec) const {
    itimerspec spec;
    memset(&spec, 0, sizeof spec);
    spec.it_value.tv_sec = start_after_sec;
    spec.it_interval.tv_sec = interval_sec;
    if (timerfd_settime(fd, 0, &spec, 0) == -1) {
        int err = errno;
        throw annotated_exception("timerfd", err);
    }
}

int timer_fd::value_of(std::initializer_list<fd_mode> mode) {
    int res = 0;
    for (auto it = mode.begin(); it != mode.end(); it++) {
        switch (*it) {
            case NONBLOCK:
                res |= TFD_NONBLOCK;
                break;
            case CLOEXEC:
                res |= TFD_CLOEXEC;
                break;
            case SIMPLE:
                res |= 0;
                break;
        }
    }
    return res;
}