#include "util.h"


std::string to_lower(std::string other) {
    std::transform(other.begin(), other.end(), other.begin(), ::tolower);
    return other;
}