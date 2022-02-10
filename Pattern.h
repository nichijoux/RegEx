#ifndef _ZH_PATTERN_H_
#define _ZH_PATTERN_H_

#include <string_view>
#include <vector>

namespace zhRegex {
    class Pattern {
    public:
        virtual ~Pattern() = default;
        //整个input字符串是否匹配pattern
        virtual bool match(std::string_view &input) = 0;
        //找出所有匹配的string
        virtual std::vector<std::string_view> contains(std::string_view &input) = 0;
    };
}  // namespace zhRegex

#endif  //_PATTERN_H_
