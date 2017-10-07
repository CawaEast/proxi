#ifndef PROXY_SERVER_H_
#define PROXY_SERVER_H_

#include <map>
#include <string>
#include <memory>
#include <list>

#include "request_processing/resolver.h"
#include "request_processing/buffered_message.h"
#include "request_processing/header_parser.h"
#include "epoll_queue/epoll_elem.h"
#include "epoll_queue/connection.h"
#include "epoll_queue/epoll_queue.h"


// Proxy server. It starts, when epoll it contains is started, and stops in destructor
struct proxy_server {

    proxy_server() = delete;

    proxy_server(int epoll_size, uint16_t port, int queue_size);

    void run();

    epoll_queue queue;

private:
    static const size_t MAX_CACHE_SIZE = 20000;

    // Types of used containers
    using cache_t = simple_cache<std::string, cached_message, MAX_CACHE_SIZE>;
    using sockets_t = std::map<int, epoll_elem>;
    using connections_t = std::list<connection>;
    using resolver_t = resolver;
    using resolved_ip_t = resolved_ip;

    // Actions used in connections handling
    template<typename... Args>
    using action_with = std::function<void(Args...)>;

    using action = action_with<>;
    using action_with_connection = action_with<typename connections_t::iterator>;
    using action_with_response = action_with<server_response>;
    using action_with_request = action_with<client_request>;

    using on_resolve_t = std::map<std::pair<int, std::string>, action_with_connection>;

    // Monadic-like functions for handling connections
    // Connect to server and do "next"
    void connect_to_server(sockets_t::iterator sock, std::string host, action_with_connection next);

    // Read message and do "next"
    template<typename T, typename C>
    void read(epoll_elem &from, buffered_message<T> message, C iterator, action_with<buffered_message<T>> next);

    // Send message and do "next"
    template<typename T, typename C>
    void send(epoll_elem &to, buffered_message<T> message, C iterator, action next);

    // Send request, read response and do "next"
    template<typename C>
    void send_and_read(epoll_elem &to, client_request, C iterator, action_with_response next);

    // Read response and send it to client during reading
    void fast_transfer(connections_t::iterator conn, client_request rqst);

    // Send response and save to cache if it's possible
    void send_server_response(connections_t::iterator conn, client_request rqst, server_response);

    // Send 404 bad request
    void send_404(sockets_t::iterator client);

    // Get client from broken connection
    sockets_t::iterator escape_client(connections_t::iterator conn);

    // Default actions

    // Connect to host from header
    action_with_request first_request_read(sockets_t::iterator client);

    // If host differs from host, then connect. Otherwise, handle request
    action reuse_connection(connections_t::iterator conn, std::string old_host);

    // Start validation or start transfer
    action_with_connection handle_client_request(client_request rqst);

    // Start raw transfer
    action handle_connect(connections_t::iterator conn);

    // Decide, can we send cached or should download response again
    action_with_response handle_validation_response(connections_t::iterator conn, client_request rqst,
                                                    server_response cached);

    // Connect to server
    epoll_core::handler_t make_server_connect_handler(connections_t::iterator conn, resolved_ip_t ip);

    epoll_core::handler_t make_connect_transfer_handler(epoll_elem &in,
                                                        std::shared_ptr<raw_message> in_message,
                                                        epoll_elem &out,
                                                        std::shared_ptr<raw_message> out_message,
                                                        connections_t::iterator conn);

    // Caching
    client_request make_validate_request(request_header rqst, response_header response) const;

    void save_cached(std::string url, cached_message const &response);

    bool is_cached(request_header const &request) const;

    cached_message get_cached(request_header const &request) const;

    void delete_cached(request_header const &request);


    resolver_t rt;
    on_resolve_t on_resolve;
    cache_t cache;

    sockets_t::iterator listener;
    sockets_t::iterator notifier;
};


#endif /* PROXY_SERVER_H_ */
