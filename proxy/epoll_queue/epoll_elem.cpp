#include "epoll_elem.h"

epoll_elem::epoll_elem() : epoll(0), fd(0) {}

epoll_elem::epoll_elem(epoll_core &epoll, file_descriptor &&fd, fd_state state) :
        epoll(&epoll), fd(std::move(fd)), events(state) {

    this->epoll->register_fd(this->fd, state);
}

epoll_elem::epoll_elem(epoll_core &epoll, file_descriptor &&fd, fd_state state, size_t timeout_in,
                       size_t cur_ticks) :
        epoll(&epoll), fd(std::move(fd)), events(state), timeout(timeout_in), expires_in(cur_ticks + timeout) {

    this->epoll->register_fd(this->fd, state);
}

epoll_elem::epoll_elem(epoll_core &epoll, file_descriptor &&fd, fd_state state,
                       epoll_core::handler_t handler) :
        epoll(&epoll), fd(std::move(fd)), events(state) {

    this->epoll->register_fd(this->fd, state, handler);
}

epoll_elem::epoll_elem(epoll_core &epoll, file_descriptor &&fd, fd_state state,
                       epoll_core::handler_t handler,
                       size_t timeout_in, size_t cur_ticks) : epoll(&epoll), fd(std::move(fd)),
                                                              events(state),
                                                              timeout(timeout_in),
                                                              expires_in(cur_ticks + timeout) {

    this->epoll->register_fd(this->fd, state, handler);
}

epoll_elem::epoll_elem(epoll_elem &&other) : epoll_elem() {
    swap(*this, other);
}

epoll_elem &epoll_elem::operator=(epoll_elem &&other) {
    swap(*this, other);
    return *this;
}


epoll_elem::~epoll_elem() {
    if (epoll != 0 && fd.get() != 0) {
        epoll->unregister_fd(fd);
    }
}

void epoll_elem::update(fd_state state) {
    if (events != state) {
        events = state;
        epoll->update_fd(fd, state);
    }
}

void epoll_elem::update(epoll_core::handler_t handler) {
    epoll->update_fd_handler(fd, handler);
}

void epoll_elem::update(fd_state state, epoll_core::handler_t handler) {
    update(state);
    update(handler);
}

void epoll_elem::change_timeout(size_t new_timeout) {
    timeout = new_timeout;
}


file_descriptor &epoll_elem::get_fd() {
    return fd;
}

file_descriptor const &epoll_elem::get_fd() const {
    return fd;
}

void swap(epoll_elem &first, epoll_elem &other) {
    using std::swap;
    swap(first.epoll, other.epoll);
    swap(first.fd, other.fd);
    swap(first.events, other.events);
    swap(first.timeout, other.timeout);
    swap(first.expires_in, other.expires_in);
}

std::string to_string(epoll_elem &er) {
    return "registration " + std::to_string(er.get_fd().get());
}

fd_state epoll_elem::get_state() const {
    return events;
}

