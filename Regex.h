#ifndef _ZH_REGEX_H_
#define _ZH_REGEX_H_

#include <string>
#include <string_view>

#include "DFA.h"

namespace zhRegex {
    //构造函数
    class Regex {
    private:
        Pattern *pattern;

    public:
        explicit Regex(Pattern *pattern);

        ~Regex();

        //整个input字符串是否匹配pattern
        bool match(const char *input);
        //整个input字符串是否匹配pattern
        bool match(std::string &input);
        //整个input字符串是否匹配pattern
        bool match(std::string_view &input);

        //找出所有匹配的string
        std::vector<std::string_view> contains(const char *input);
        //找出所有匹配的string
        std::vector<std::string_view> contains(std::string &input);
        //找出所有匹配的string
        std::vector<std::string_view> contains(std::string_view &input);
    };
}  // namespace zhRegex
#endif