#ifndef PROXY_SERVER_EPOLL_WRAP_H
#define PROXY_SERVER_EPOLL_WRAP_H

#include <memory>
#include <sys/epoll.h>
#include <map>
#include "../util/file_descriptor.h"
#include "fd_state.h"

struct epoll_core : file_descriptor {
    using handler_t = std::function<void(fd_state)>;
    using handlers_t = std::map<int, handler_t>;

    epoll_core(int max_queue_size);

    epoll_core(epoll_core &&other);

    // Register file descriptor in epoll
    void register_fd(const file_descriptor &fd, fd_state events);

    void register_fd(const file_descriptor &fd, fd_state events, handler_t handler);

    // Unregister
    void unregister_fd(const file_descriptor &fd);

    // Update state (and handler) of file descriptor
    void update_fd(const file_descriptor &fd, fd_state events);

    void update_fd_handler(const file_descriptor &fd, handler_t handler);

    // Start epoll
    void start_wait();

    // Say epoll that it should stop
    void stop_wait();

    friend void swap(epoll_core &first, epoll_core &second);

private:
    epoll_event create_event(int fd, fd_state const &events);

    int queue_size;
    std::unique_ptr<epoll_event[]> events;
    handlers_t handlers;
    volatile bool started, stopped;
};


#endif //PROXY_SERVER_EPOLL_WRAP_H
