#include "Lexer.h"

namespace zhRegex {
    //构造函数
    Lexer::Lexer(std::string_view &pattern) : pattern(pattern) {}

    //获取下一个token
    void Lexer::advance() {
        if (index >= pattern.size()) {
            currentToken = RegExToken::Eof;
            currentChar = '\0';
            return;
        }
        currentChar = pattern[index++];
        //如果跟着转义字符则调用escapeHandler(),否则调用semanticHandler()
        if (currentChar == '\\') {
            currentToken = escapeHandler();
        } else {
            currentToken = semanticHandler();
        }
    }

    //处理转义字符
    RegExToken Lexer::escapeHandler() {
        //部分转义字符
        static char escapeCharacter[] = {
            'd',  //代表数字
            'D',  //代表非数字
            'w',  //代表字符
            'W'   //代表非字符
        };
        currentChar = pattern[index++];
        for (char c : escapeCharacter) {
            if (c == currentChar) {
                return RegExToken::EscapeChar;
            }
        }
        return RegExToken::SingleChar;
    }

    //处理普通字符
    RegExToken Lexer::semanticHandler() const {
        if (regexTokens.find(currentChar) == regexTokens.end()) {
            return RegExToken::SingleChar;
        }
        return regexTokens[currentChar];
    }

}  // namespace zhRegex