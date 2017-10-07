#ifndef UTIL_H_
#define UTIL_H_

#include <iostream>
#include <string>
#include <string.h>
#include <algorithm>
#include <map>

inline std::string to_string(std::string const &message) {
    return message;
}

inline std::string to_string(char *message) {
    return std::string(message);
}

template<typename S, typename T>
void log(S const &tag, T const &message) {
    using ::to_string;
    using std::to_string;
    std::cout << to_string(tag) << ": " << to_string(message) << '\n';
}


std::string to_lower(std::string str);


#endif /* UTIL_H_ */
