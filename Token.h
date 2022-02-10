#ifndef _ZH_TOKEN_H_
#define _ZH_TOKEN_H_

#include <map>
#include <unordered_map>
#include <unordered_set>

#include "RegexException.h"

#ifndef CHAR_MIN
#define CHAR_MIN -128
#endif

#ifndef CHAR_MAX
#define CHAR_MAX 127
#endif

namespace zhRegex {
    //重命名unordered_set
    template <typename T>
    using hashSet = std::unordered_set<T>;
    //重命名unordered_map
    template <class K, class V, class Hash = std::hash<K>, class Equal = std::equal_to<K>>
    using hashMap = std::unordered_map<K, V, Hash, Equal>;
    // Token
    enum class RegExToken {
        Eof,              //结束标识
        AnyChar,          //任意字符,.
        Positive,         //正闭包,+
        Kleene,           //克林闭包,*
        Or,               //或,|
        Question,         //问号,?
        LeftParen,        //左括号,(
        RightParen,       //右括号,)
        LeftCollection,   //左中括号,[
        RightCollection,  //右中括号,]
        LeftBrace,        //左大括号,{
        RightBrace,       //右大括号,}
        CharBegin,        //以某char开头,^
        CharEnd,          //以某char结尾,$
        Dash,             //破折号,-
        SingleChar,       //单个字符
        EscapeChar        //转义字符
    };

    static std::unordered_map<char, RegExToken> initialRegexToken() {
        std::unordered_map<char, RegExToken> tokens;
        tokens['.'] = RegExToken::AnyChar;
        tokens['^'] = RegExToken::CharBegin;
        tokens['$'] = RegExToken::CharEnd;
        tokens['('] = RegExToken::LeftParen;
        tokens[')'] = RegExToken::RightParen;
        tokens['['] = RegExToken::LeftCollection;
        tokens[']'] = RegExToken::RightCollection;
        tokens['{'] = RegExToken::LeftBrace;
        tokens['}'] = RegExToken::RightBrace;
        tokens['-'] = RegExToken::Dash;
        tokens['+'] = RegExToken::Positive;
        tokens['*'] = RegExToken::Kleene;
        tokens['|'] = RegExToken::Or;
        tokens['?'] = RegExToken::Question;
        return tokens;
    }

    static std::unordered_map<char, RegExToken> regexTokens = initialRegexToken();
}  // namespace zhRegex
#endif