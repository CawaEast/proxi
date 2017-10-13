#ifndef PROXY_SERVER_EPOLL_REGISTRATION_H
#define PROXY_SERVER_EPOLL_REGISTRATION_H


#include "epoll_core.h"

struct epoll_elem {
    epoll_elem();

    epoll_elem(epoll_core &epoll, file_descriptor &&fd, fd_state state);

    epoll_elem(epoll_core &epoll, file_descriptor &&fd, fd_state state, size_t timeout, size_t cur_ticks);

    epoll_elem(epoll_core &epoll, file_descriptor &&fd, fd_state state, epoll_core::handler_t handler);

    epoll_elem(epoll_core &epoll, file_descriptor &&fd, fd_state state, epoll_core::handler_t handler,
               size_t timeout, size_t cur_ticks);

    epoll_elem(epoll_elem &&other);

    epoll_elem &operator=(epoll_elem &&other);

    ~epoll_elem();

    file_descriptor &get_fd();

    file_descriptor const &get_fd() const;

    fd_state get_state() const;

    void update(fd_state state);

    void update(epoll_core::handler_t handler);

    void update(fd_state state, epoll_core::handler_t handler);



    void change_timeout(size_t new_timeout);

    friend void swap(epoll_elem &first, epoll_elem &other);

    friend std::string to_string(epoll_elem &er);

    epoll_core *epoll; // It's here because of epoll_elem should be move-constructable
    file_descriptor fd;
    fd_state events;
    size_t timeout;
    size_t expires_in;
};


#endif //PROXY_SERVER_EPOLL_REGISTRATION_H
