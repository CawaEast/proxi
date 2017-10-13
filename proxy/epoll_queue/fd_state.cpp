#include "fd_state.h"


fd_state::fd_state() :
        fd_st(0) {
}

fd_state::fd_state(uint32_t st) :
        fd_st(st) {
}

fd_state::fd_state(state st) :
        fd_state({st}) {
}

fd_state::fd_state(std::initializer_list<state> st) : fd_st(value_of(st)) {

}

fd_state::fd_state(fd_state const &other) :
        fd_st(other.fd_st) {
}

fd_state::fd_state(fd_state &&other) {
    swap(*this, other);
}

fd_state &fd_state::operator=(fd_state other) {
    swap(*this, other);
    return *this;
}

bool fd_state::operator!=(fd_state other) const {
    return fd_st != other.fd_st;
}

bool fd_state::is(fd_state st) const {
    return (fd_st & st.fd_st) != 0;
}

uint32_t fd_state::get() const {
    return fd_st;
}

fd_state operator^(fd_state first, fd_state second) {
    return fd_state(first.get() ^ second.get());
}

fd_state operator|(fd_state first, fd_state second) {
    return fd_state(first.get() | second.get());
}

uint32_t fd_state::value_of(std::initializer_list<state> st) {
    uint32_t res = 0;
    for (auto it = st.begin(); it != st.end(); it++) {
        switch (*it) {
            case IN:
                res |= EPOLLIN;
                break;
            case OUT:
                res |= EPOLLOUT;
                break;
            case ERROR:
                res |= EPOLLERR;
                break;
            case HUP:
                res |= EPOLLHUP;
                break;
            case RDHUP:
                res |= EPOLLRDHUP;
                break;
            default:
                res |= 0;
        }
    }
    return res;
}

bool operator==(fd_state const &first, fd_state const &second) {
    return first.fd_st == second.fd_st;
}

void swap(fd_state &first, fd_state &second) {
    std::swap(first.fd_st, second.fd_st);
}