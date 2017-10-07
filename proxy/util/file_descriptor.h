#ifndef PROXY_SERVER_FILE_DESCRIPTOR_H
#define PROXY_SERVER_FILE_DESCRIPTOR_H

#include <unistd.h>
#include <string>

struct file_descriptor {
    explicit file_descriptor(int fd);

    file_descriptor(file_descriptor &&other);

    file_descriptor &operator=(file_descriptor &&other);

    virtual ~file_descriptor();

    // Get id of file descriptor
    int get() const;

    // Can we read from it
    int can_read() const;

    long read(void *message, size_t message_size) const;

    long write(void const *message, size_t message_size) const;

    friend void swap(file_descriptor &first, file_descriptor &second);

    friend std::string to_string(file_descriptor const &fd);

protected:
    file_descriptor();

    int fd;
};


#endif //PROXY_SERVER_FILE_DESCRIPTOR_H
