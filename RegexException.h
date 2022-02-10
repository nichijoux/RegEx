#ifndef _ZH_REGEX_EXCEPTION_H_
#define _ZH_REGEX_EXCEPTION_H_

#include <exception>

namespace zhRegex {
    struct RegexException : public std::exception {
        [[nodiscard]] const char* what() const noexcept override;
    };
}  // namespace zhRegex
#endif  // !_REGEX_EXCEPTION_H_
