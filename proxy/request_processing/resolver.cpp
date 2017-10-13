//
// Created by cawa on 10/7/17.
//

#include "resolver.h"


resolved_ip::resolved_ip() :
        ips(), port(0), extra() {
}

resolved_ip::resolved_ip(ips_t ips, uint16_t port, resolver_extra &&extra) :
        ips(std::move(ips)), port(port), extra(std::move(extra)) {
}


void swap(resolved_ip &first, resolved_ip &other) {
    using std::swap;
    swap(first.ips, other.ips);
    swap(first.port, other.port);
    swap(first.extra, other.extra);
}

resolved_ip::resolved_ip(resolved_ip const &other) : ips(other.ips), extra(other.extra) {
}

resolved_ip::resolved_ip(resolved_ip &&other) : resolved_ip() {
    swap(*this, other);
}

resolved_ip &resolved_ip::operator=(resolved_ip other) {
    swap(*this, other);
    return *this;
}

bool resolved_ip::has_ip() const {
    return !ips.empty();
}

adress_t resolved_ip::get_ip() const {
    if (ips.empty()) {
        return adress_t();
    }
    uint32_t ip = *ips.begin();

    return {ip, port};
}

void resolved_ip::next_ip() {
    if (!ips.empty()) {
        ips.pop_front();
    }

}

resolver_extra &resolved_ip::get_extra() {
    return extra;
}

resolver_extra const &resolved_ip::get_extra() const {
    return extra;
}


resolver::resolver() :  should_stop(false) {
    // Ignoring signals from other threads
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    for (size_t i = 0; i < THREAD_COUNT; i++) {
        threads[i] = thread_wrap(&resolver::main_loop, this);
    }

    // Don't ignore in main thread
    sigemptyset(&set);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
}

resolver::~resolver() {
    stop();
}

void resolver::stop() {
    should_stop = true;
    cv.notify_all();
}

void resolver::resolve_host(std::string host, file_descriptor const &notifier, resolver_extra extra) {
    {
        std::lock_guard<std::mutex> lg(in_mutex);
        in_queue.push({host, &notifier, std::move(extra)});
    }
    cv.notify_one();
}

resolved_ip resolver::get_ip() {
    resolved_ip res;
    {
        std::lock_guard<std::mutex> lg(out_mutex);
        res = std::move(out_queue.front());
        out_queue.pop();
    }
    return res;
}

typename resolver::ips_t resolver::find_cached(std::string const &host) {
    std::lock_guard<std::mutex> lg(cache_mutex);
    if (cache.has(host)) {
        return cache.find(host);
    }
    return ips_t();
}

void resolver::cache_ip(std::string const &host, typename resolver::ips_t ip) {
    std::lock_guard<std::mutex> lg(cache_mutex);
    cache.insert(host, ip);
}

typename resolver::ips_t resolver::resolve_ip(std::string host, std::string port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int code;
    struct addrinfo *addr;
    if ((code = getaddrinfo(host.c_str(), port.c_str(), &hints, &addr)) != 0) {
        throw annotated_exception("resolver", gai_strerror(code));
    }
    ips_t ips;
    struct addrinfo *cur = addr;
    while (cur != 0) {
        sockaddr_in *ep = reinterpret_cast<sockaddr_in *>(cur->ai_addr);
        ips.push_back(ep->sin_addr.s_addr);
        cur = cur->ai_next;
    }
    freeaddrinfo(addr);
    cache_ip(host, ips);
    return ips;
}

void resolver::main_loop() {
    while (true) {
        if (should_stop) {
            break;
        }
        {
            std::unique_lock<std::mutex> lock(in_mutex);

            if (in_queue.empty()) {
                cv.wait(lock);
                continue;
            }
        }

        in_query p{};
        {
            std::lock_guard<std::mutex> lg(in_mutex);
            if (in_queue.size() == 0) {
                continue;
            }
            p = std::move(in_queue.front());
            in_queue.pop();
        }

        size_t pos = p.host.find(":");
        std::string host, port;
        if (pos == std::string::npos) {
            host = p.host;
            port = "80";
        } else {
            host = p.host.substr(0, pos);
            port = p.host.substr(pos + 1);
        }

        ips_t ips = find_cached(host);

        if (ips.empty()) {
            try {
                ips = resolve_ip(host, port);
            } catch (annotated_exception const &e) {
                log(e);
                ips = ips_t();
            }
        }
        resolved_ip tmp(ips, htons((uint16_t) std::stoi(port)), std::move(p.extra));

        {
            std::lock_guard<std::mutex> lg(out_mutex);
            out_queue.push(std::move(tmp));
        }
        uint64_t u = 1;
        p.notifier->write(&u, sizeof(uint64_t));
    }
}
