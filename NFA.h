#ifndef _ZH_NFA_H_
#define _ZH_NFA_H_

#include <memory>
#include <stack>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "Lexer.h"
#include "Pattern.h"
#include "Token.h"

/*
正则表达式对于BNF范式有
regexExpression ::= ("(" groupExpression ")")*
groupExpression ::= ("(" expression ")")[*+?]
expression ::= factorConnect ("|" factorConnect)*
factorConnect ::= factor | factor·factor*
factor ::= (("(")("^")term("$")(")") | ("(")("^")term("*" | "+" | "?" | "{n,m}")($)(")"))*
term ::= char | "[" char "-" char "]" | .
*/

namespace zhRegex {
    class DFA;

    struct DFAMapHash;
    struct DFAMapEqual;

    enum class NFAEdgeType {
        eofEdge,        //无边,为终结
        epslion,        // 1或2条epslion边
        normalChar,     //普通单词
        charCollection  //单词集合
    };

    // NFA节点
    struct NFANode {
        char edgeValue;
        NFAEdgeType edgeType;
        std::shared_ptr<hashSet<char>> edgeSet;
        std::shared_ptr<NFANode> next1;
        std::shared_ptr<NFANode> next2;
        //此weak_ptr专门用于+和*闭包会出现的环形,否则会导致循环引用
        std::weak_ptr<NFANode> loop;

        NFANode();

        explicit NFANode(NFAEdgeType edgeType);

        NFANode(const NFANode &node);

        void inverseCharSet();
    };

    struct NFANodePair {
        std::shared_ptr<NFANode> start;
        std::shared_ptr<NFANode> end;
    };

    //非确定有限状态机
    class NFA : public Pattern {
        //友元DFA
        friend class DFA;

    private:
        Lexer lexer;
        // NFA头结点
        std::shared_ptr<NFANode> head;

        //单个字符
        void singleChar(NFANodePair &pair);
        //任意字符即.
        void anyChar(NFANodePair &pair);
        //字符集
        void charCollection(NFANodePair &pair);
        //转义字符
        void escapeChar(NFANodePair &pair);

        //*闭包
        void kleeneClosure(NFANodePair &pair);
        //+闭包
        void positiveClosure(NFANodePair &pair);
        //?闭包
        void questionClosure(NFANodePair &pair);
        //{n,m}闭包
        void repeatClosure(NFANodePair &pair);

        //{n,m}闭包辅助函数,m = -2时表示不存在,m = -1时表示无限
        void repeatClosureHelper(NFANodePair &pair, int n, int m);
        void regexExpression(NFANodePair &pair);
        void groupExpression(NFANodePair &pair);
        void expression(NFANodePair &pair);
        void factorConnect(NFANodePair &pair);
        void factor(NFANodePair &pair);
        void term(NFANodePair &pair);

        // type是否可由构成factor
        bool canFactor(RegExToken type);
        // closure算法(见虎书P26)
        static hashSet<NFANode *> closure(hashSet<NFANode *> &closureSet);
        // DFAedge算法(见虎书P27)
        static hashSet<NFANode *> DFAedge(hashSet<NFANode *> &closureSet, char c);

    public:
        explicit NFA(const char *pattern);
        explicit NFA(std::string &pattern);
        explicit NFA(std::string_view &pattern);
        ~NFA() override = default;

        //获取DFA
        DFA NFAToDFA();
        //整个input字符串是否匹配pattern
        bool match(std::string_view &input) override;
        //找出所有匹配的string
        std::vector<std::string_view> contains(std::string_view &input) override;
    };
}  // namespace zhRegex
#endif