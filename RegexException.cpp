#include "RegexException.h"

namespace zhRegex {
    const char* RegexException::what() const noexcept {
        return "Regular Expression Exception!";
    }
}  // namespace zhRegex