#include "connection.h"

connection::connection() : timeout(0), expires_in(0) {}

connection::connection(epoll_elem &&client, epoll_elem &&server, size_t timeout, size_t ticks) :
        timeout(timeout), expires_in(ticks + timeout), client(std::move(client)), server(std::move(server)) {}

socket_wrap const &connection::get_client() const {
    return *static_cast<socket_wrap const *>(&client.get_fd());
}

socket_wrap const &connection::get_server() const {
    return *static_cast<socket_wrap const *>(&server.get_fd());
}

epoll_elem &connection::get_client_registration() {
    return client;
}

epoll_elem &connection::get_server_registration() {
    return server;
}

void swap(connection &first, connection &second) {
    using std::swap;
    swap(first.client, second.client);
    swap(first.server, second.server);
    swap(first.timeout, second.timeout);
    swap(first.expires_in, second.expires_in);
}

std::string to_string(connection const &conn) {
    using std::to_string;
    std::string res = "connection " +
                      to_string(conn.get_client().get()) +
                      " <-> " +
                      to_string(conn.get_server().get());
    return res;
}

void connection::change_timeout(size_t new_timeout) {
    timeout = new_timeout;
}

std::string to_string(std::list<connection>::iterator const &iterator) {
    return to_string(*iterator);
}