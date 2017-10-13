#ifndef PROXY_SERVER_QUEUE_H
#define PROXY_SERVER_QUEUE_H

#include "epoll_core.h"
#include "connection.h"
#include "epoll_elem.h"

static const int DEFAULT_QUEUE_SIZE = 200;
static const size_t TICK_INTERVAL = 2;
static const size_t SHORT_SOCKET_TIMEOUT = 60 * 4;
static const size_t LONG_SOCKET_TIMEOUT = 60 * 20;
static const size_t INFINITE_TIMEOUT = (size_t) 1 << (4 * sizeof(size_t));
//static const size_t SHORT_SOCKET_TIMEOUT = (size_t) 1 << (4 * sizeof(size_t));
//static const size_t LONG_SOCKET_TIMEOUT = (size_t) 1 << (4 * sizeof(size_t));

struct epoll_queue {

    using sockets_t = std::map<int, epoll_elem>;
    using connections_t = std::list<connection>;


    epoll_core epoll;
    connections_t connections;
    sockets_t sockets;
    sockets_t::iterator timer;
    size_t ticks = 0;

    epoll_queue() = delete;

    epoll_queue(int size);

    sockets_t::iterator save_registration(epoll_elem registration, size_t timeout);

    sockets_t::iterator save_registration(file_descriptor &&fd, fd_state state, size_t timeout);

    sockets_t::iterator
    save_registration(file_descriptor &&fd, fd_state state, size_t timeout, epoll_core::handler_t handler);

    connection make_connection(epoll_elem client, epoll_elem server, size_t timeout);

    connections_t::iterator save_connection(connection conn);

    void close(connections_t::iterator connection);

    void close(sockets_t::iterator socket);

    void set_active(sockets_t::iterator iterator);

    void set_active(connections_t::iterator iterator);

};

std::string to_string(epoll_queue::sockets_t::iterator const &iterator);


#endif //PROXY_SERVER_QUEUE_H
