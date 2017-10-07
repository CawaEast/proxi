#ifndef PROXY_SERVER_FD_STATE_H
#define PROXY_SERVER_FD_STATE_H


#include <algorithm>

struct fd_state {
    enum state {
        IN, OUT, WAIT, ERROR, HUP, RDHUP
    };

    fd_state();

    fd_state(uint32_t st);

    fd_state(state st);

    fd_state(std::initializer_list<state> st);

    fd_state(fd_state const &other);

    fd_state(fd_state &&other);

    fd_state &operator=(fd_state other);

    bool operator!=(fd_state other) const;

    bool is(fd_state st) const;

    uint32_t get() const;

    friend fd_state operator^(fd_state first, fd_state second);

    friend fd_state operator|(fd_state first, fd_state second);

    friend void swap(fd_state &first, fd_state &second);

    friend bool operator==(fd_state const &first, fd_state const &second);

private:
    static uint32_t value_of(std::initializer_list<state> st);

    uint32_t fd_st;
};


#endif //PROXY_SERVER_FD_STATE_H
