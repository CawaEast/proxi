#include <sys/signalfd.h>
#include "proxy_server.h"
#include "util/event_fd.h"


proxy_server::proxy_server(int epoll_size, uint16_t port, int queue_size) : queue(epoll_size), rt() {

    socket_wrap listener(socket_wrap::NONBLOCK);
    event_fd notifier(0, event_fd::SEMAPHORE);

    listener.bind(port);
    listener.listen(queue_size);

    auto listener_handler = [this](fd_state state) {
        if (state.is(fd_state::IN)) {
            socket_wrap &listener = *static_cast<socket_wrap *>(&this->listener->second.get_fd());
            try {
                socket_wrap client = listener.accept(socket_wrap::NONBLOCK);
                log("new client accepted", client.get());
                sockets_t::iterator it = this->queue.save_registration(std::move(client), fd_state::IN,
                                                                       SHORT_SOCKET_TIMEOUT);
                read(it->second, client_request(), it, first_request_read(it));
            } catch (annotated_exception const &e) {
                log("accept failed", e.what());
                return;
            }
        }
    };

    auto notifier_handler = [this](fd_state state) {
        if (state.is(fd_state::IN)) {
            uint64_t u;
            file_descriptor &notifier = this->notifier->second.get_fd();
            notifier.read(&u, sizeof(uint64_t));

            socket_wrap destination(socket_wrap::NONBLOCK);

            resolved_ip_t ip = this->rt.get_ip();

            on_resolve_t::iterator it = on_resolve.find({ip.get_extra().socket, ip.get_extra().host});
            sockets_t::iterator client = this->queue.sockets.find(ip.get_extra().socket);
            if (it == on_resolve.end()) {
                // Client disconnected during resolving of ip
                log(client, "client disconnected during resolving of ip");
                return;
            }

            if (!ip.has_ip()) {
                log(client, "address " + ip.get_extra().host + " not found");
                on_resolve.erase(it);
                send_404(client);
                return;
            }

            try {
                destination.connect(ip.get_ip());
            } catch (annotated_exception const &e) {
                if (e.get_errno() != EINPROGRESS) {
                    log(e);
                    send_404(client);
                    on_resolve.erase(it);
                    return;
                }
            }

            connection conn = queue.make_connection(std::move(client->second),
                                                    epoll_elem(this->queue.epoll, std::move(destination),
                                                               fd_state::OUT), SHORT_SOCKET_TIMEOUT);

            this->queue.sockets.erase(client);
            log(conn, "ip for " + ip.get_extra().host + " resolved: " + to_string(ip.get_ip()));

            connections_t::iterator conn_it = this->queue.save_connection(std::move(conn));

            resolver_extra ip_extra = ip.get_extra();
            conn_it->get_client_registration().update(fd_state::RDHUP,
                                                      [this, ip_extra, conn_it](fd_state state) {
                                                          // If client disconnect while we haven't connected to server
                                                          if (state.is(fd_state::RDHUP)) {
                                                              log(conn_it, "client dropped connection");
                                                              auto it = on_resolve.find(
                                                                      {ip_extra.socket, ip_extra.host});
                                                              if (it != on_resolve.end()) {
                                                                  on_resolve.erase(it);
                                                              }
                                                              this->queue.close(conn_it);
                                                          }
                                                      });

            conn_it->get_server_registration().update(make_server_connect_handler(conn_it, ip));
        }
    };


    this->notifier = queue.save_registration(std::move(notifier), fd_state::IN, INFINITE_TIMEOUT,
                                             notifier_handler);
    this->listener = queue.save_registration(std::move(listener), fd_state::IN, INFINITE_TIMEOUT,
                                             listener_handler);

}


proxy_server::action_with_request proxy_server::first_request_read(sockets_t::iterator client) {
    return [this, client](client_request rqst) {
        std::string host = rqst.get_header().get_property("host");
        connect_to_server(client, host, handle_client_request(rqst));
    };
}


void proxy_server::connect_to_server(sockets_t::iterator sock, std::string host, action_with_connection do_next) {
    socket_wrap &s = *static_cast<socket_wrap *>(&sock->second.get_fd());
    log(sock, "establishing connection to " + host);
    on_resolve.insert({{s.get(), host}, do_next});

    // If socket disconnected during resolving, stop resolving
    sock->second.update(fd_state::RDHUP, [this, sock, host](fd_state state) {
        if (state.is(fd_state::RDHUP)) {
            log(sock, "disconnected during resolving of IP");
            on_resolve.erase(on_resolve.find({sock->second.get_fd().get(), host}));
            this->queue.close(sock);
        }
    });

    rt.resolve_host(host, notifier->second.get_fd(), {s.get(), host});
}

proxy_server::action_with_connection proxy_server::handle_client_request(client_request rqst) {
    return [this, rqst](connections_t::iterator conn) {
        if (rqst.get_header().get_request_line().get_type() == request_line::GET) {

            // If cached, validate
            if (is_cached(rqst.get_header())) {
                log(conn, "found cached for " + to_url(rqst.get_header()) + ", validating...");

                server_response cached = get_cached(rqst.get_header());
                send_and_read(conn->get_server_registration(),
                              make_validate_request(rqst.get_header(), cached.get_header()),
                              conn, handle_validation_response(conn, rqst, cached));
                return;
            }
        }
        if (rqst.get_header().get_request_line().get_type() == request_line::CONNECT) {
            server_response resp(response_header(response_line(200, "Connection Established")), "");
            send(conn->get_client_registration(), resp, conn, handle_connect(conn));
            return;
        }
        fast_transfer(conn, rqst);
    };
}

proxy_server::action_with_response proxy_server::handle_validation_response(connections_t::iterator conn,
                                                                            client_request rqst,
                                                                            server_response cached) {
    return [this, rqst, conn, cached](server_response resp) mutable {
        int code = resp.get_header().get_request_line().get_code();

        if (code == 200 || code == 304) {
            // Can send cached
            log(conn, "cache valid");
            if (resp.get_header().has_property("connnection")) {
                cached.get_header().set_property("connection", resp.get_header().get_property("connection"));
            }

            send_server_response(conn, std::move(rqst), std::move(cached));
        } else {
            // Can't do it
            log(conn, "cache invalid");

            delete_cached(rqst.get_header());

            // Send data to server
            if (resp.get_header().has_property("connection") &&
                to_lower(resp.get_header().get_property("connection")).compare("close") == 0) {
                // Re-connect if closed
                log(conn, "server closed due to \"Connection = close\", reconnecting");

                epoll_elem client = std::move(conn->get_client_registration());
                queue.close(conn);
                sockets_t::iterator it = queue.save_registration(std::move(client), LONG_SOCKET_TIMEOUT);

                connect_to_server(it,
                                  rqst.get_header().get_property("host"),
                                  [this, rqst](connections_t::iterator conn) {
                                      fast_transfer(conn, rqst);
                                  });
            } else {
                fast_transfer(conn, rqst);
            }
        }
    };
}


proxy_server::action proxy_server::reuse_connection(connections_t::iterator conn, std::string old_host) {
    return [this, conn, old_host]() {
        log(conn, "server response sent");
        log(conn, "kept alive");

        conn->get_server_registration().update(fd_state::RDHUP, [this, conn](fd_state state) {
            if (state.is(fd_state::RDHUP)) {
                log(conn, "server dropped connection");
                queue.close(conn);
                return;
            }
        });

        // Read client request
        read<request_header>(conn->get_client_registration(), client_request(), conn,
                             [this, conn, old_host](client_request rqst) {

                                 std::string host = rqst.get_header().get_property("host");
                                 log(conn, "client reused: " + old_host + " -> " + host);

                                 if (host.compare(old_host) == 0) {
                                     // If host the same, handle request
                                     handle_client_request(std::move(rqst))(conn);
                                 } else {
                                     // Otherwise, disconnect, connect and send
                                     log(conn, "disconnect from " + old_host);
                                     epoll_elem client = std::move(conn->get_client_registration());
                                     queue.close(conn);

                                     sockets_t::iterator it = queue.save_registration(std::move(client),
                                                                                      LONG_SOCKET_TIMEOUT);
                                     connect_to_server(it, host, handle_client_request(std::move(rqst)));
                                 }
                             });

    };
}

proxy_server::action proxy_server::handle_connect(connections_t::iterator conn) {
    return [this, conn]() {
        log(conn, "CONNECT started");
        std::shared_ptr<raw_message> client_message = std::make_shared<raw_message>();
        std::shared_ptr<raw_message> server_message = std::make_shared<raw_message>();
        conn->get_server_registration()
                .update({fd_state::RDHUP, fd_state::IN, fd_state::OUT},
                        make_connect_transfer_handler(conn->get_server_registration(), server_message,
                                                      conn->get_client_registration(), client_message, conn));
        conn->get_client_registration()
                .update({fd_state::RDHUP, fd_state::IN, fd_state::OUT},
                        make_connect_transfer_handler(conn->get_client_registration(), client_message,
                                                      conn->get_server_registration(), server_message, conn));
    };
}

epoll_core::handler_t proxy_server::make_connect_transfer_handler(epoll_elem &in,
                                                                  std::shared_ptr<raw_message> in_message,
                                                                  epoll_elem &out,
                                                                  std::shared_ptr<raw_message> out_message,
                                                                  std::list<connection>::iterator conn) {

    return [this, &in, in_message, &out, out_message, conn](fd_state state) {
        queue.set_active(conn);

        if (state.is(fd_state::RDHUP)) {
            log(conn, "CONNECT stopped");
            this->queue.close(conn);
            return;
        }

        if (state.is(fd_state::IN) && in_message->can_read()) {
            try {
                in_message->read_from(in.get_fd());
            } catch (annotated_exception const &e) {
                log(conn, e.what());
                this->queue.close(conn);
            }
            if (!in_message->can_read()) {
                in.update(in.get_state() ^ fd_state::IN);
            }
            if (in_message->can_write()) {
                out.update(out.get_state() | fd_state::OUT);
            }
        }
        if (state.is(fd_state::OUT) && out_message->can_write()) {
            try {
                out_message->write_to(in.get_fd());
            } catch (annotated_exception const &e) {
                log(conn, e.what());
                this->queue.close(conn);
            }
            if (!out_message->can_write()) {
                in.update(in.get_state() ^ fd_state::OUT);
            }
            if (out_message->can_read()) {
                out.update(out.get_state() | fd_state::IN);
            }
        }
    };
}

epoll_core::handler_t proxy_server::make_server_connect_handler(connections_t::iterator conn, resolved_ip_t ip) {
    return [this, conn, ip](fd_state state) mutable {
        socket_wrap const &server = conn->get_server();
        queue.set_active(conn);

        auto query = on_resolve.find({ip.get_extra().socket, ip.get_extra().host});
        if (state.is(fd_state::RDHUP)) {
            log(conn, "connection to " + ip.get_extra().host +
                      ": server " + std::to_string(server.get()) + "dropped connection");
            on_resolve.erase(query);
            send_404(escape_client(conn));
            this->queue.close(conn);
            return;
        }

        if (state.is({fd_state::HUP, fd_state::ERROR})) {
            int code;
            socklen_t size = sizeof(code);
            server.get_option(SO_ERROR, &code, &size);

            annotated_exception e(to_string(conn) + ": connect", code);
            if (code == ENETUNREACH || code == ECONNREFUSED) {
                adress_t old_ip = ip.get_ip();
                ip.next_ip();
                if (!ip.has_ip()) {
                    log(conn, "connection to " + ip.get_extra().host + ": no relevant ip, closing");
                    on_resolve.erase(query);
                    send_404(escape_client(conn));
                    this->queue.close(conn);
                    return;
                }
                log(conn, "connection to " + ip.get_extra().host + ": ip "
                          + to_string(old_ip) + " isn't valid. Trying " + to_string(ip.get_ip()));
                adress_t cur_ip = ip.get_ip();

                try {
                    server.connect(cur_ip);
                } catch (annotated_exception const &e) {
                    if (e.get_errno() != EINPROGRESS) {
                        // bad error
                        log(e);
                        log(conn, "closing");
                        on_resolve.erase(query);
                        send_404(escape_client(conn));
                        this->queue.close(conn);
                        return;
                    }
                }
                return;
            } else {
                log(e);
                on_resolve.erase(query);
                send_404(escape_client(conn));
                this->queue.close(conn);
                return;
            }
        }

        if (state.is(fd_state::OUT)) {
            log(conn, "established");

            action_with_connection action = query->second;
            on_resolve.erase(query);

            conn->get_client_registration().update(fd_state::WAIT);
            conn->get_server_registration().update(fd_state::WAIT);
            conn->change_timeout(LONG_SOCKET_TIMEOUT);

            action(conn);
        }
    };
}

typename proxy_server::sockets_t::iterator proxy_server::escape_client(connections_t::iterator conn) {
    epoll_elem client = std::move(conn->get_client_registration());
    return this->queue.save_registration(std::move(client), SHORT_SOCKET_TIMEOUT);
}

template<typename T, typename C>
void proxy_server::read(epoll_elem &from, buffered_message<T> message, C iterator,
                        action_with<buffered_message<T>> next) {
    std::shared_ptr<buffered_message<T>> s_message = std::make_shared<buffered_message<T>>(std::move(message));
    from.update({fd_state::IN, fd_state::RDHUP},
                [this, &from, s_message, iterator, next](fd_state state) {
                    file_descriptor const &fd = from.get_fd();
                    queue.set_active(iterator);

                    if (state.is(fd_state::RDHUP)) {
                        if (fd.can_read() == 0) {
                            log(iterator, "disconnected");
                            queue.close(iterator);
                            return;
                        }
                    }

                    if (state.is({fd_state::HUP, fd_state::ERROR})) {
                        int code;
                        socklen_t size = sizeof(code);
                        socket_wrap const &sock = *static_cast<socket_wrap const *>(&fd);
                        sock.get_option(SO_ERROR, &code, &size);
                        annotated_exception exception(to_string(iterator) + " read", code);

                        log(exception);
                        queue.close(iterator);
                    }

                    if (state.is(fd_state::IN)) {
                        try {
                            s_message->read_from(fd);
                        } catch (annotated_exception const &e) {
                            log(iterator, e.what());
                            queue.close(iterator);
                            return;
                        }
                        if (s_message->is_read()) {
                            from.update(fd_state::WAIT);
                            next(std::move(*s_message));
                        }
                    }
                });
}

template<typename T, typename C>
void proxy_server::send(epoll_elem &to, buffered_message<T> message,
                        C iterator, action next) {
    std::shared_ptr<buffered_message<T>> s_message = std::make_shared<buffered_message<T>>(std::move(message));
    to.update({fd_state::OUT, fd_state::RDHUP},
              [this, &to, s_message, iterator, next](fd_state state) {
                  file_descriptor const &fd = to.get_fd();
                  queue.set_active(iterator);

                  if (state.is(fd_state::RDHUP)) {
                      log(iterator, "disconnected");
                      queue.close(iterator);
                      return;
                  }

                  if (state.is({fd_state::HUP, fd_state::ERROR})) {
                      int code;
                      socklen_t size = sizeof(code);
                      socket_wrap const &sock = *static_cast<socket_wrap const *>(&fd);
                      sock.get_option(SO_ERROR, &code, &size);
                      annotated_exception exception(to_string(iterator) + " send", code);
                      log(exception);
                      queue.close(iterator);
                  }

                  if (state.is(fd_state::OUT)) {
                      try {
                          s_message->write_to(fd);
                      } catch (annotated_exception const &e) {
                          log(iterator, e.what());
                          queue.close(iterator);
                          return;
                      }

                      if (s_message->is_written()) {
                          to.update(fd_state::WAIT);
                          next();
                      }
                  }
              });
}

template<typename C>
void proxy_server::send_and_read(epoll_elem &to, client_request rqst,
                                 C iterator, action_with_response next) {
    send(to, rqst, iterator, [this, &to, iterator, next]() {
        log(iterator, "request sent");
        read(to, server_response(), iterator, next);
    });
}

void proxy_server::send_server_response(connections_t::iterator conn, client_request rqst, server_response resp) {
    log(conn, "server's response read");
    bool closed = resp.get_header().has_property("connection") &&
                  to_lower(resp.get_header().get_property("connection")).compare("close") == 0;

    if (closed) {
        log(conn, "server closed due to \"Connection = close\" ");
        sockets_t::iterator it = escape_client(conn);
        this->queue.close(conn);

        send(it->second, resp, it, [this, it]() {
            log(it->second.get_fd(), "server response sent");
            log(it->second.get_fd(), "closed due to \"Connection = close\"");
            this->queue.close(it);
        });
    } else {
        conn->get_server_registration().update(fd_state::WAIT);
        send(conn->get_client_registration(), std::move(resp), conn,
             reuse_connection(conn, rqst.get_header().get_property("host")));
    }

}

void proxy_server::send_404(sockets_t::iterator client) {
    response_header header(response_line(404, "Not Found"));

    server_response response(std::move(header), "");
    send(client->second, std::move(response), client, [this, client]() {
        this->queue.close(client);
    });
}

void proxy_server::fast_transfer(connections_t::iterator conn, client_request rqst) {
    send(conn->get_server_registration(), rqst, conn, [this, conn, rqst]() {
        std::shared_ptr<client_request> s_rqst = std::make_shared<client_request>(std::move(rqst));
        std::shared_ptr<server_response> resp = std::make_shared<server_response>(server_response());

        conn->get_server_registration().update(
                {fd_state::IN, fd_state::RDHUP}, [this, conn, s_rqst, resp](fd_state state) {
                    queue.set_active(conn);
                    file_descriptor const &server = conn->get_server();

                    if (state.is(fd_state::RDHUP)) {
                        if (server.can_read() == 0) {
                            log(conn, "server dropped connection");
                            this->queue.close(conn);
                            return;
                        }
                    }

                    if (state.is({fd_state::HUP, fd_state::ERROR})) {
                        int code;
                        socklen_t size = sizeof(code);
                        socket_wrap const &sock = *static_cast<socket_wrap const *>(&server);
                        sock.get_option(SO_ERROR, &code, &size);
                        annotated_exception exception(to_string(conn) + " send", code);
                        log(exception);
                        this->queue.close(conn);
                    }

                    if (state.is(fd_state::IN)) {
                        try {
                            resp->read_from(server);
                        } catch (annotated_exception const &e) {
                            log(conn, e.what());
                            this->queue.close(conn);
                            return;
                        }

                        if (resp->can_write()) {
                            conn->get_client_registration().update({fd_state::OUT, fd_state::RDHUP});
                        }

                        if (resp->is_read()) {
                            std::string url = to_url(s_rqst->get_header());
                            if (should_cache(resp->get_header())) {
                                save_cached(url, resp->get_cache());
                                log(conn, "response from " + url + " saved to cache");
                            }

                            send_server_response(conn, std::move(*s_rqst), std::move(*resp));
                        }
                    }
                });
        conn->get_client_registration().update(
                {fd_state::WAIT, fd_state::RDHUP}, [this, conn, s_rqst, resp](fd_state state) {
                    file_descriptor const &fd = conn->get_client();
                    queue.set_active(conn);

                    if (state.is(fd_state::RDHUP)) {
                        log(conn, "client dropped connection");
                        this->queue.close(conn);
                        return;
                    }

                    if (state.is({fd_state::HUP, fd_state::ERROR})) {
                        int code;
                        socklen_t size = sizeof(code);
                        socket_wrap const &sock = *static_cast<socket_wrap const *>(&fd);
                        sock.get_option(SO_ERROR, &code, &size);
                        annotated_exception exception(to_string(conn) + " send", code);
                        log(exception);
                        this->queue.close(conn);
                    }

                    if (state.is(fd_state::OUT) && resp->can_write()) {
                        try {
                            resp->write_to(fd);
                        } catch (annotated_exception const &e) {
                            log(conn, e.what());
                            this->queue.close(conn);
                            return;
                        }
                        if (!resp->can_write()) {
                            conn->get_client_registration().update(
                                    conn->get_client_registration().get_state() ^ fd_state::OUT);
                        }
                    }
                });
    });
}


void proxy_server::save_cached(std::string url, cached_message const &response) {
    cache.insert(std::move(url), response);
}

bool proxy_server::is_cached(request_header const &request) const {
    return cache.has(to_url(request));
}

cached_message proxy_server::get_cached(request_header const &request) const {
    return cache.find(to_url(request));
}

void proxy_server::delete_cached(request_header const &request) {
    cache.erase(to_url(request));
}

client_request proxy_server::make_validate_request(request_header rqst, response_header response) const {
    return client_request(make_validate_header(rqst, response), "");
}

void proxy_server::run() {
    queue.epoll.start_wait();
}



