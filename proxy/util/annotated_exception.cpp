#include "annotated_exception.h"

annotated_exception::annotated_exception() noexcept : message(""), errnum(0) {

}

annotated_exception::annotated_exception(annotated_exception const &other) noexcept : message(other.message),
                                                                                      errnum(other.errnum) {

}

annotated_exception::annotated_exception(std::string tag, int errnum) : errnum(errnum) {
    char buffer[BUFFER_LENGTH];
    message = tag;
    message.append(": ");
    message.append(std::string(strerror_r(errnum, buffer, BUFFER_LENGTH)));
}

annotated_exception::annotated_exception(std::string tag, std::string message) : message(tag + ": " + message),
                                                                                 errnum(-1) {
}

int annotated_exception::get_errno() const {
    return errnum;
}

const char *annotated_exception::what() const noexcept {
    return message.c_str();
}

void log(annotated_exception const &e) {
    log("ERROR", e.what());
}
