#include "file_descriptor.h"
#include <sys/ioctl.h>
#include "annotated_exception.h"


file_descriptor::file_descriptor() :
        fd(0) {
}

file_descriptor::file_descriptor(int fd) :
        fd(fd) {
    if (fd == -1) {
        int err = errno;
        throw annotated_exception("fd", err);
    }
}

file_descriptor::file_descriptor(file_descriptor &&other) : fd() {
    swap(*this, other);
}

file_descriptor &file_descriptor::operator=(file_descriptor &&other) {
    swap(*this, other);
    return *this;
}

file_descriptor::~file_descriptor() {
    if (fd != 0) {
        close(fd);
    }
}

int file_descriptor::get() const {
    return fd;
}

int file_descriptor::can_read() const {
    int bytes = 0;
    if (ioctl(fd, FIONREAD, &bytes) == -1) {
        int err = errno;
        throw annotated_exception("can_read", err);
    }
    return bytes;
}

long file_descriptor::read(void *message, size_t message_size) const {
    long read = ::read(fd, message, message_size);
    if (read == -1) {
        int err = errno;
        throw annotated_exception("read", err);
    }
    return read;
}

long file_descriptor::write(void const *message, size_t message_size) const {
    long written = ::write(fd, message, message_size);
    if (written == -1) {
        int err = errno;
        throw annotated_exception("write", err);
    }
    return written;
}

void swap(file_descriptor &first, file_descriptor &second) {
    std::swap(first.fd, second.fd);
}

std::string to_string(file_descriptor const &fd) {
    return "file descriptor " + std::to_string(fd.get());
}