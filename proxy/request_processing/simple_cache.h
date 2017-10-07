#ifndef PROXY_SERVER_SIMPLE_CACHE_H
#define PROXY_SERVER_SIMPLE_CACHE_H


#include "../util/annotated_exception.h"
#include <cstring>
#include <map>
#include <iostream>
#include <string>
#include <string.h>
#include <algorithm>
#include <map>

template<typename K, typename V, size_t MAX_SIZE>
struct simple_cache {
    simple_cache();

    simple_cache(simple_cache const &other) = default;

    simple_cache(simple_cache &&other) = default;

    simple_cache &operator=(simple_cache const &other) = default;

    simple_cache &operator=(simple_cache &&other) = default;


    void insert(K key, V value);

    bool has(K key) const;

    V &find(K key);

    V const &find(K key) const;

    void erase(K key);

    size_t size() const;

private:
    struct node;
    using values_t = std::map<K, node>;

    struct node {
        V value;
        typename values_t::iterator next;
        typename values_t::iterator prev;

        node(V value) : value(std::move(value)) {}

        node(node const &other) = default;

        node(node &&other) = default;

        node &operator=(node const &other) = default;

        node &operator=(node &&other) = default;
    };

    values_t values;
    typename values_t::iterator first, last;
};


template<typename K, typename V, size_t MAX_SIZE>
simple_cache<K, V, MAX_SIZE>::simple_cache() : values{}, first(values.begin()), last(values.end()) {
}

template<typename K, typename V, size_t MAX_SIZE>
void simple_cache<K, V, MAX_SIZE>::insert(K key, V value) {
    if (values.size() == MAX_SIZE) {
        auto next = first->second.next;
        values.erase(first);
        first = next;
        next->second.prev = next;
    }
    auto it = values.insert(std::make_pair(std::move(key), node(std::move(value))));
    if (it.second) {
        auto inserted = it.first;
        if (values.size() == 1) {
            first = inserted;
            inserted->second.prev = inserted;
        } else {
            inserted->second.prev = last;
            last->second.next = inserted;
        }
        last = inserted;
        inserted->second.next = inserted;
    }
}

template<typename K, typename V, size_t MAX_SIZE>
bool simple_cache<K, V, MAX_SIZE>::has(K key) const {
    auto it = values.find(std::move(key));
    return it != values.cend();
}

template<typename K, typename V, size_t MAX_SIZE>
V &simple_cache<K, V, MAX_SIZE>::find(K key) {
    auto it = values.find(std::move(key));
    if (it == values.end()) {
        throw annotated_exception("simple cache", "element not found");
    }
    return it->second.value;
}

template<typename K, typename V, size_t MAX_SIZE>
V const &simple_cache<K, V, MAX_SIZE>::find(K key) const {
    auto it = values.find(std::move(key));
    if (it == values.cend()) {
        throw annotated_exception("simple cache", "element not found");
    }
    return it->second.value;
}

template<typename K, typename V, size_t MAX_SIZE>
void simple_cache<K, V, MAX_SIZE>::erase(K key) {
    auto it = values.find(std::move(key));
    if (it == values.end()) {
        return;
    }
    auto prev = it->second.prev;
    auto next = it->second.next;
    if (it == first) {
        first = next;
    } else {
        prev->second.next = next;
    }
    if (it == last) {
        last = prev;
    } else {
        next->second.prev = prev;
    }
    values.erase(it);
}

template<typename K, typename V, size_t MAX_SIZE>
size_t simple_cache<K, V, MAX_SIZE>::size() const {
    return values.size();
}


#endif //PROXY_SERVER_SIMPLE_CACHE_H
