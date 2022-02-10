#include "Regex.h"

namespace zhRegex {
    //构造函数
    Regex::Regex(Pattern *pattern) {
        this->pattern = pattern;
    }

    Regex::~Regex() {
        this->pattern = nullptr;
    }

    //整个input字符串是否匹配pattern
    bool Regex::match(const char *input) {
        std::string_view inputS = input;
        return pattern->match(inputS);
    }
    //整个input字符串是否匹配pattern
    bool Regex::match(std::string &input) {
        std::string_view inputS = input;
        return pattern->match(inputS);
    }
    //整个input字符串是否匹配pattern
    bool Regex::match(std::string_view &input) {
        return pattern->match(input);
    }

    //找出所有匹配的string
    std::vector<std::string_view> Regex::contains(const char *input) {
        std::string_view inputS = input;
        return contains(inputS);
    }
    //找出所有匹配的string
    std::vector<std::string_view> Regex::contains(std::string &input) {
        std::string_view inputS = input;
        return pattern->contains(inputS);
    }
    //找出所有匹配的string
    std::vector<std::string_view> Regex::contains(std::string_view &input) {
        return pattern->contains(input);
    }
}  // namespace zhRegex