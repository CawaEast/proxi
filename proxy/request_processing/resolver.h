#ifndef RESOLVER_H_
#define RESOLVER_H_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <netdb.h>
#include <string>
#include <utility>
#include <memory>
#include <deque>
#include <utility>
#include <signal.h>
#include <pthread.h>
#include <atomic>
#include <functional>

#include "../util/socket_wrap.h"
#include "../util/util.h"
#include "simple_cache.h"
#include "../util/annotated_exception.h"



struct resolver_extra {
    int socket;
    std::string host;
};

// Extra information passed with ips adress through resolver
struct resolved_ip {
    using ips_t = std::deque<uint32_t>;
    friend struct resolver;

    resolved_ip();

    resolved_ip(ips_t ips, uint16_t port, resolver_extra &&extra);

    resolved_ip(resolved_ip const &other);

    resolved_ip(resolved_ip &&other);

    resolved_ip &operator=(resolved_ip other);

    ~resolved_ip() = default;

    bool has_ip() const;

    adress_t get_ip() const;

    void next_ip();

    resolver_extra &get_extra();

    resolver_extra const &get_extra() const;

    friend void swap(resolved_ip &first, resolved_ip &second);

private:

    ips_t ips;
    uint16_t port;
    resolver_extra extra;

};


void swap(resolved_ip &first, resolved_ip &second);

// Safe RAII wrap for threads. Starts in constructor, joins in destructor
struct thread_wrap {
    thread_wrap() = default;

    template<class Fn, class... Args>
    thread_wrap(Fn &&func, Args &&... args) : thread(std::forward<Fn>(func), std::forward<Args>(args)...) {};

    thread_wrap(thread_wrap const &other) = default;

    thread_wrap(thread_wrap &&other) = default;

    thread_wrap &operator=(thread_wrap const &other) = default;

    thread_wrap &operator=(thread_wrap &&other) = default;

    ~thread_wrap() {
        if (thread.joinable()) {
            thread.join();
        }
    }

private:
    std::thread thread;
};

// Multi-thread (4 threads) resolver for ip adresses. After resolving, returns resolved IP with extra,
// passed in resolve_host();

struct resolver {
    friend struct resolved_ip;

    resolver();

    resolver(resolver &&other) = delete;

    resolver(resolver const &other) = delete;

    resolver &operator=(resolver &&other) = delete;

    resolver &operator=(resolver const &other) = delete;

    ~resolver();

    // Forcibly stop threads of resolver
    void stop();

    // Resolve host and notify passed file_descriptor, that IP is resolved
    void resolve_host(std::string host, file_descriptor const &notifier, resolver_extra extra);

    // Get resolved IP with extra, passed in resolve_host
    resolved_ip get_ip();

private:
    static const size_t THREAD_COUNT = 4;
    static const size_t CACHE_SIZE = 500;

    void main_loop();

    struct in_query {
        std::string host;
        file_descriptor const *notifier;
        resolver_extra extra;
    };

    using ips_t = typename resolved_ip::ips_t;

    ips_t find_cached(std::string const &host);

    void cache_ip(std::string const &host, ips_t ips);

    ips_t resolve_ip(std::string host, std::string port);

    std::queue<in_query> in_queue;
    std::queue<resolved_ip> out_queue;

    simple_cache<std::string, ips_t, CACHE_SIZE> cache;

    std::atomic_bool should_stop;
    std::mutex in_mutex, out_mutex, cache_mutex;
    std::condition_variable cv;
    thread_wrap threads[THREAD_COUNT];
};

#endif /* RESOLVER_H_ */
