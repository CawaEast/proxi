#ifndef PROXY_SERVER_ANNOTATED_EXCEPTION_H
#define PROXY_SERVER_ANNOTATED_EXCEPTION_H

#include <exception>
#include <string>
#include <memory.h>
#include "util.h"


class annotated_exception : public std::exception {
public:
    annotated_exception() noexcept;

    annotated_exception(annotated_exception const &other) noexcept;

    annotated_exception(std::string tag, std::string message);

    annotated_exception(std::string tag, int errnum);

    virtual ~annotated_exception() noexcept = default;

    virtual const char *what() const noexcept override;

    int get_errno() const;

private:
    static const size_t BUFFER_LENGTH = 64;
    std::string message;
    int errnum;
};

void log(annotated_exception const &e);


#endif //PROXY_SERVER_ANNOTATED_EXCEPTION_H
