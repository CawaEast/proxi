#include "epoll_core.h"
#include "../util/annotated_exception.h"


epoll_core::epoll_core(int max_queue_size) :
        file_descriptor(), queue_size(max_queue_size), events(
        new epoll_event[max_queue_size]), handlers{}, started{false}, stopped{
        true} {
    fd = epoll_create(1);
    if (fd == -1) {
        int err = errno;
        throw annotated_exception("epoll_create", err);
    }
}

epoll_core::epoll_core(epoll_core &&other) {
    swap(*this, other);
}

epoll_event epoll_core::create_event(int fd, fd_state const &st) {
    epoll_event event;
    memset(&event, 0, sizeof event);
    event.data.fd = fd;
    event.events = st.get();
    return event;
}

void epoll_core::register_fd(const file_descriptor &fd, fd_state events) {
    epoll_event event = create_event(fd.get(), events);

    if (epoll_ctl(this->fd, EPOLL_CTL_ADD, fd.get(), &event)) {
        int err = errno;
        throw annotated_exception("epoll register", err);
    }
}

void epoll_core::register_fd(const file_descriptor &fd, fd_state events,
                             handler_t handler) {
    register_fd(fd, events);
    handlers.insert({fd.get(), handler});
}

void epoll_core::unregister_fd(const file_descriptor &fd) {
    if (epoll_ctl(this->fd, EPOLL_CTL_DEL, fd.get(), 0)) {
        int err = errno;
        throw annotated_exception("epoll_unregister", err);
    }
    handlers.erase(fd.get());
}

void epoll_core::update_fd(const file_descriptor &fd, fd_state events) {
    epoll_event event = create_event(fd.get(), events);
    if (epoll_ctl(this->fd, EPOLL_CTL_MOD, fd.get(), &event)) {
        int err = errno;
        throw annotated_exception("epoll_update", err);
    }
}

void epoll_core::update_fd_handler(const file_descriptor &fd, epoll_core::handler_t handler) {
    handlers.erase(fd.get());
    handlers.insert({fd.get(), handler});
}

void epoll_core::start_wait() {
    if (started) {
        return;
    }
    started = true;
    stopped = false;

    while (!stopped) {
        int events_number = epoll_wait(fd, events.get(), queue_size, -1);
        if (events_number == -1) {
            int err = errno;
            if (err == EINTR) {
                break;
            }
            throw annotated_exception("epoll_wait", err);
        }

        for (int i = 0; i < events_number; i++) {
            int fd = events[i].data.fd;
            uint32_t state = events[i].events;
            handlers_t::iterator it = handlers.find(fd);
            if (it != handlers.end()) {
                handler_t handler = it->second;
                handler(fd_state(state));
            }
            if (stopped) {
                break;
            }
        }
    }
    started = false;
}

void epoll_core::stop_wait() {
    stopped = true;
}

void swap(epoll_core &first, epoll_core &second) {
    using std::swap;
    swap(first.fd, second.fd);
    swap(first.queue_size, second.queue_size);
    swap(first.started, second.started);
    swap(first.stopped, second.stopped);
    swap(first.events, second.events);
    swap(first.handlers, second.handlers);
}