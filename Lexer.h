#ifndef _ZH_LEXER_H_
#define _ZH_LEXER_H_

#include <string_view>

#include "Token.h"

namespace zhRegex {
    class Lexer {
    private:
        std::string_view pattern;
        int index = 0;
        RegExToken currentToken = RegExToken::Eof;
        char currentChar = '\0';

        //处理转义字符
        RegExToken escapeHandler();

        //处理普通字符
        RegExToken semanticHandler() const;

    public:
        Lexer() = default;

        explicit Lexer(std::string_view &pattern);

        //获取下一个token
        void advance();

        //是否匹配
        inline bool match(RegExToken token) const {
            return currentToken == token;
        }

        //获取当前token
        inline RegExToken getCurrentToken() const {
            return currentToken;
        }

        //获取当前text
        inline char getCurrentChar() const {
            return currentChar;
        }
    };
}  // namespace zhRegex

#endif