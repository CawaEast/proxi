#include <sys/signalfd.h>
#include <csignal>
#include <map>
#include "epoll_queue.h"
#include "timer_fd.h"
#include "../util/signal_fd.h"
#include "../util/annotated_exception.h"

using sockets_t = std::map<int, epoll_elem>;
using connections_t = std::list<connection>;

epoll_queue::epoll_queue(int epoll_size) : epoll(epoll_size) {

    signal_fd sig_fd({SIGINT, SIGPIPE}, {signal_fd::SIMPLE});
    epoll_elem signal_registration(epoll, std::move(sig_fd), fd_state::IN);
    signal_registration.update([&signal_registration, this](fd_state state) mutable {
        if (state.is(fd_state::IN)) {
            struct signalfd_siginfo sinf;
            long size = signal_registration.get_fd().read(&sinf, sizeof(struct signalfd_siginfo));
            if (size != sizeof(struct signalfd_siginfo)) {
                return;
            }
            if (sinf.ssi_signo == SIGINT) {
                log("\nserver", "stopped");
                epoll.stop_wait();
            }
        }
    });

    timer_fd timer(timer_fd::MONOTONIC, timer_fd::SIMPLE);
    timer.set_interval(TICK_INTERVAL, TICK_INTERVAL);
    auto timer_handler = [this](fd_state state) {
        if (state.is(fd_state::IN)) {
            file_descriptor &timer_in = this->timer->second.get_fd();
            uint64_t ticked = 0;
            timer_in.read(&ticked, sizeof ticked);

            ticks += ticked * TICK_INTERVAL;
            for (auto it = sockets.begin(); it != sockets.end();) {
                if (it->second.expires_in <= ticks) {
                    log(it, "closed due timeout");
                    it = sockets.erase(it);
                } else {
                    it++;
                }
            }

            ticks += ticked;
            for (auto it = connections.begin(); it != connections.end();) {
                if (it->expires_in <= ticks) {
                    log(it, "closed due timeout");
                    it = connections.erase(it);
                } else {
                    it++;
                }
            }
        }
    };
    this->timer = save_registration(epoll_elem(epoll, std::move(timer), fd_state::IN, timer_handler),
                                    INFINITE_TIMEOUT);
}

sockets_t::iterator epoll_queue::save_registration(epoll_elem registration, size_t timeout) {
    registration.expires_in = ticks + timeout;
    registration.timeout = timeout;
    int fd = registration.get_fd().get();
    return sockets.insert(std::make_pair(fd, epoll_elem(std::move(registration)))).first;
}


sockets_t::iterator epoll_queue::save_registration(file_descriptor &&socket, fd_state state, size_t timeout) {
    int fd = socket.get();
    return sockets.insert(
            std::make_pair(fd, epoll_elem(epoll, std::move(socket), state, timeout, ticks))).first;
}

sockets_t::iterator
epoll_queue::save_registration(file_descriptor &&socket, fd_state state, size_t timeout,
                               epoll_core::handler_t handler) {
    int fd = socket.get();
    return sockets.insert(
            std::make_pair(fd, epoll_elem(epoll, std::move(socket), state, handler, timeout, ticks))).first;
}

void epoll_queue::close(sockets_t::iterator socket) {
    sockets.erase(socket);
}

connections_t::iterator epoll_queue::save_connection(connection conn) {
    return connections.insert(connections.end(), std::move(conn));
}

void epoll_queue::close(connections_t::iterator connection) {
    connections.erase(connection);
}

void epoll_queue::set_active(sockets_t::iterator iterator) {
    iterator->second.expires_in = ticks + iterator->second.timeout;
}

void epoll_queue::set_active(connections_t::iterator iterator) {
    iterator->expires_in = ticks + iterator->timeout;
}

connection epoll_queue::make_connection(epoll_elem client, epoll_elem server, size_t timeout) {
    return connection(std::move(client), std::move(server), timeout, ticks);
}

std::string to_string(epoll_queue::sockets_t::iterator const &iterator) {
    return to_string(iterator->second);
}

