#ifndef PROXY_SERVER_CONNECTION_H
#define PROXY_SERVER_CONNECTION_H


#include <list>
#include "epoll_elem.h"
#include "../util/socket_wrap.h"

struct connection {
    connection();

    connection(epoll_elem &&client, epoll_elem &&server, size_t timeout, size_t ticks);

    connection(connection &&other) = default;

    connection &operator=(connection &&other) = default;

    socket_wrap const &get_client() const;

    socket_wrap const &get_server() const;

    epoll_elem &get_client_registration();

    epoll_elem &get_server_registration();

    friend void swap(connection &first, connection &second);

    friend std::string to_string(connection const &conn);

    void change_timeout(size_t new_timeout);

    size_t timeout, expires_in;

private:
    epoll_elem client, server;
};

std::string to_string(std::list<connection>::iterator const &iterator);

#endif //PROXY_SERVER_CONNECTION_H
