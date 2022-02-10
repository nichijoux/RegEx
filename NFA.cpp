#include "NFA.h"

#include "DFA.h"

namespace zhRegex {
    // class NFANode
    NFANode::NFANode() {
        edgeValue = '\0';
        edgeType = NFAEdgeType::eofEdge;
        edgeSet = nullptr;
    }

    NFANode::NFANode(NFAEdgeType edgeType) {
        this->edgeValue = '\0';
        this->edgeType = edgeType;
        this->edgeSet = nullptr;
        if (edgeType == NFAEdgeType::charCollection)
            edgeSet = std::make_shared<hashSet<char>>();
    }

    NFANode::NFANode(const NFANode &node) {
        edgeValue = node.edgeValue;
        edgeType = node.edgeType;
        edgeSet = nullptr;
        if (node.edgeSet != nullptr) {
            edgeSet = std::make_shared<hashSet<char>>(*node.edgeSet);
        }
        if (node.next1 != nullptr) {
            next1 = node.next1;
        }
        if (node.next2 != nullptr) {
            next2 = node.next2;
        }
        if (node.loop.lock() != nullptr) {
            loop = node.loop;
        }
    }

    void NFANode::inverseCharSet() {
        if (edgeSet == nullptr)
            return;
        hashSet<char> originSet;
        originSet.swap(*edgeSet);
        for (int c = CHAR_MIN; c < CHAR_MAX; ++c) {
            if (originSet.find(c) == originSet.end())
                edgeSet->emplace(c);
        }
    }

    // class NFA
    NFA::NFA(const char *pattern) {
        std::string_view patternS = pattern;
        this->lexer = Lexer(patternS);
        NFANodePair pair;
        lexer.advance();
        regexExpression(pair);
        this->head = pair.start;
    }

    NFA::NFA(std::string &pattern) {
        std::string_view patternS = pattern;
        this->lexer = Lexer(patternS);
        NFANodePair pair;
        lexer.advance();
        regexExpression(pair);
        this->head = pair.start;
    }

    NFA::NFA(std::string_view &pattern) {
        this->lexer = Lexer(pattern);
        NFANodePair pair;
        lexer.advance();
        regexExpression(pair);
        this->head = pair.start;
    }

    //单个字符,其情况包罗万象
    void NFA::singleChar(NFANodePair &pair) {
        pair.start = std::make_shared<NFANode>(NFAEdgeType::normalChar);
        pair.end = std::make_shared<NFANode>();
        pair.start->next1 = pair.end;
        pair.start->edgeValue = lexer.getCurrentChar();

        lexer.advance();
    }

    //.字符
    void NFA::anyChar(NFANodePair &pair) {
        if (!lexer.match(RegExToken::AnyChar))
            return;
        pair.start = std::make_shared<NFANode>(NFAEdgeType::charCollection);
        pair.start->edgeValue = '.';
        pair.end = std::make_shared<NFANode>();
        pair.start->next1 = pair.end;
        lexer.advance();
    }

    //字符集
    void NFA::charCollection(NFANodePair &pair) {
        //如果是字符集则currentToken应该为[
        if (!lexer.match(RegExToken::LeftCollection))
            return;
        bool needReverse = false;
        lexer.advance();
        if (lexer.match(RegExToken::CharBegin))
            needReverse = true;
        pair.start = std::make_shared<NFANode>(NFAEdgeType::charCollection);
        pair.end = std::make_shared<NFANode>();
        pair.start->next1 = pair.end;
        char first = '\0';
        std::shared_ptr<hashSet<char>> &nodeSet = pair.start->edgeSet;
        while (!lexer.match(RegExToken::RightCollection)) {
            //如果遇到破折号
            if (lexer.match(RegExToken::Dash)) {
                //查看前一个字符是否为字母或数字,否则应该视为正常的-符号
                if (('a' <= first && first <= 'z') ||
                    ('0' <= first && first <= '9') ||
                    ('A' <= first && first <= 'Z')) {
                    lexer.advance();
                    char last = lexer.getCurrentChar();
                    for (char c = first; c <= last; ++c) nodeSet->emplace(c);
                } else {
                    first = lexer.getCurrentChar();
                    nodeSet->emplace(first);
                }
            } else {
                first = lexer.getCurrentChar();
                nodeSet->emplace(first);
            }
            lexer.advance();
        }
        if (needReverse)
            pair.start->inverseCharSet();
        lexer.advance();
    }

    //转义字符
    void NFA::escapeChar(NFANodePair &pair) {
        if (!lexer.match(RegExToken::EscapeChar))
            return;
        static char escapeCharacter[] = {
            'd',  //代表数字
            'D',  //代表非数字
            'w',  //代表字符
            'W'   //代表非字符
        };
        bool isCharSetFlag = false;
        char currentChar = lexer.getCurrentChar();
        for (char c : escapeCharacter) {
            if (c == currentChar) {
                isCharSetFlag = true;
                break;
            }
        }
        NFAEdgeType type = isCharSetFlag ? NFAEdgeType::charCollection : NFAEdgeType::normalChar;
        pair.start = std::make_shared<NFANode>(type);
        pair.end = std::make_shared<NFANode>();
        pair.start->next1 = pair.end;
        if (isCharSetFlag) {
            std::shared_ptr<hashSet<char>> &nodeSet = pair.start->edgeSet;
            if (currentChar == 'd' || currentChar == 'D') {
                for (char c = '0'; c <= '9'; ++c)
                    nodeSet->emplace(c);
                if (currentChar == 'D')
                    pair.start->inverseCharSet();
            } else if (currentChar == 'w' || currentChar == 'W') {
                for (char c = 'a'; c <= 'z'; ++c)
                    nodeSet->emplace(c);
                for (char c = 'A'; c <= 'Z'; ++c)
                    nodeSet->emplace(c);
                if (currentChar == 'W')
                    pair.start->inverseCharSet();
            }
        }
        lexer.advance();
    }

    // term ::= char | "[" char "-" char "]" | .
    void NFA::term(NFANodePair &pair) {
        switch (lexer.getCurrentToken()) {
        case RegExToken::AnyChar:
            anyChar(pair);
            break;
        case RegExToken::LeftCollection:
            charCollection(pair);
            break;
        case RegExToken::EscapeChar:
            escapeChar(pair);
            break;
        default:
            singleChar(pair);
            break;
        }
    }

    //*闭包
    void NFA::kleeneClosure(NFANodePair &pair) {
        std::shared_ptr<NFANode> start = std::make_shared<NFANode>(NFAEdgeType::epslion);
        std::shared_ptr<NFANode> end = std::make_shared<NFANode>();
        start->next1 = pair.start;
        start->next2 = end;

        //由于*闭包会导致成环,因此一定要注意应该使用loop指针
        pair.end->loop = pair.start;
        pair.end->next1 = end;
        pair.end->edgeType = NFAEdgeType::epslion;

        pair.start = start;
        pair.end = end;

        lexer.advance();
    }

    //+闭包
    void NFA::positiveClosure(NFANodePair &pair) {
        std::shared_ptr<NFANode> start = std::make_shared<NFANode>(NFAEdgeType::epslion);
        std::shared_ptr<NFANode> end = std::make_shared<NFANode>();
        start->next1 = pair.start;

        //由于+闭包会导致成环,因此一定要注意应该使用loop指针
        pair.end->loop = pair.start;
        pair.end->next1 = end;
        pair.end->edgeType = NFAEdgeType::epslion;

        pair.start = start;
        pair.end = end;
        lexer.advance();
    }

    //?闭包
    void NFA::questionClosure(NFANodePair &pair) {
        std::shared_ptr<NFANode> start = std::make_shared<NFANode>(NFAEdgeType::epslion);
        std::shared_ptr<NFANode> end = std::make_shared<NFANode>();

        start->next1 = pair.start;
        start->next2 = end;

        pair.end->next1 = end;
        pair.end->edgeType = NFAEdgeType::epslion;

        pair.start = start;
        pair.end = end;
        lexer.advance();
    }

    //{n,m}闭包
    void NFA::repeatClosure(NFANodePair &pair) {
        if (!lexer.match(RegExToken::LeftBrace))
            return;
        // n,m闭包
        int n = 0;
        //计算n
        while (true) {
            if (lexer.match(RegExToken::RightBrace))
                break;
            else if (lexer.match(RegExToken::SingleChar)) {
                char c = lexer.getCurrentChar();
                if ('0' <= c && c <= '9') {
                    n = n * 10 + c - '0';
                } else {
                    if (c != ',')
                        throw RegexException();
                    else
                        break;
                }
            }
            lexer.advance();
        }
        //此时为遇到逗号或者右大括号
        if (lexer.match(RegExToken::RightBrace)) {
            //如果此时为右大括号,则说明正则为{n}形式
            repeatClosureHelper(pair, n, -2);
        } else {
            //否则此时应该为lexer.getCurrentChar()应该为逗号
            int m = 0;
            lexer.advance();
            if (lexer.match(RegExToken::RightBrace)) {
                //说明此时 m 应该为无穷
                repeatClosureHelper(pair, n, -1);
            } else {
                //否则应该计算m
                while (!lexer.match(RegExToken::RightBrace)) {
                    char c = lexer.getCurrentChar();
                    if ('0' <= c && c <= '9')
                        m = m * 10 + c - '0';
                    else
                        throw RegexException();
                    lexer.advance();
                }
                repeatClosureHelper(pair, n, m);
            }
        }
        lexer.advance();
    }

    //{n,m}闭包辅助函数,m = -2时表示不存在,m = -1时表示无限
    void NFA::repeatClosureHelper(NFANodePair &pair, int n, int m) {
        if (m > 0 && n > m) {
            throw RegexException();
        }
        if (n == 0 && m == -1) {
            //等价*闭包
            kleeneClosure(pair);
        } else if (n == 1 && m == -1) {
            //等价+闭包
            positiveClosure(pair);
        } else if (n == 0 && m == 1) {
            //等价?闭包
            questionClosure(pair);
        } else {
            if (n == 0 && m == 0) {
                pair.start = nullptr;
                pair.end = nullptr;
                return;
            }
            //{n,m}形态
            //保存start
            std::shared_ptr<NFANode> saveHead = pair.start;
            std::shared_ptr<NFANode> prevTail;
            NFAEdgeType type = pair.start->edgeType;
            for (int i = 1; i < n; i++) {
                std::shared_ptr<NFANode> newStart = std::make_shared<NFANode>(type);
                newStart->edgeType = type;
                newStart->edgeValue = pair.start->edgeValue;
                newStart->edgeSet = pair.start->edgeSet;

                std::shared_ptr<NFANode> newEnd = std::make_shared<NFANode>();
                newStart->next1 = newEnd;

                //将新的term和以前的term连接
                pair.end->next1 = newStart;
                pair.end->edgeType = NFAEdgeType::epslion;

                //更新pair
                if (i == n - 1) {
                    prevTail = pair.end;
                }
                pair.start = newStart;
                pair.end = newEnd;
            }
            //{n,m}形态(n不可能为负数,m为-2或-1时不会运行以下代码)
            for (int i = n; i < m; i++) {
                std::shared_ptr<NFANode> newStart = std::make_shared<NFANode>(type);
                newStart->edgeType = type;
                newStart->edgeValue = pair.start->edgeValue;
                newStart->edgeSet = pair.start->edgeSet;

                std::shared_ptr<NFANode> newEnd = std::make_shared<NFANode>(NFAEdgeType::epslion);
                newStart->next1 = newEnd;

                // epslion边
                std::shared_ptr<NFANode> newHead = std::make_shared<NFANode>(NFAEdgeType::epslion);
                std::shared_ptr<NFANode> newTail = std::make_shared<NFANode>();

                newHead->next1 = newStart;
                newHead->next2 = newTail;
                newEnd->next1 = newTail;

                //连接新term和老term
                pair.end->edgeType = NFAEdgeType::epslion;
                pair.end->next1 = newHead;

                //更新pair
                pair.end = newTail;
            }
            if (m == -1) {
                pair.end->edgeType = NFAEdgeType::epslion;
                pair.end->loop = prevTail;
                //需要新构造一个tail
                std::shared_ptr<NFANode> tail = std::make_shared<NFANode>();
                pair.end->next1 = tail;
                pair.end = tail;
            }
            //最后更新tail和head
            pair.start = saveHead;
        }
    }

    // factor ::= (("(")("^")term("$")(")") | ("(")("^")term("*" | "+" | "?" | "{n,m}")($)(")"))*
    void NFA::factor(NFANodePair &pair) {
        try {
            //先看是否存在左括号
            if (lexer.match(RegExToken::LeftParen))
                lexer.advance();
            //^符号
            if (lexer.match(RegExToken::CharBegin))
                lexer.advance();

            term(pair);
            //闭包符
            switch (lexer.getCurrentToken()) {
            case RegExToken::Kleene:
                //*闭包
                kleeneClosure(pair);
                break;
            case RegExToken::Positive:
                //+闭包
                positiveClosure(pair);
                break;
            case RegExToken::Question:
                //?闭包
                questionClosure(pair);
                break;
            case RegExToken::LeftBrace:
                //{n,m}闭包
                repeatClosure(pair);
                break;
            default:
                break;
            }

            //是否存在右括号
            if (lexer.match(RegExToken::RightParen))
                lexer.advance();
            //$符号
            if (lexer.match(RegExToken::CharEnd))
                lexer.advance();
        } catch (RegexException &e) {
            printf("%s", e.what());
            return;
        }
    }

    //能够构建factor的符号
    bool NFA::canFactor(RegExToken type) {
        static RegExToken symbol[] = {
            RegExToken::LeftParen,
            RegExToken::LeftCollection,
            RegExToken::LeftBrace,
            RegExToken::CharBegin,
            RegExToken::SingleChar,
            RegExToken::AnyChar,
            RegExToken::Dash,
            RegExToken::EscapeChar};
        for (auto i : symbol) {
            if (type == i)
                return true;
        }
        return false;
    }

    // factorConnect ::= factor | factor·factor*
    void NFA::factorConnect(NFANodePair &pair) {
        if (canFactor(lexer.getCurrentToken()))
            factor(pair);
        while (canFactor(lexer.getCurrentToken())) {
            NFANodePair childPair;
            if (lexer.getCurrentChar() == '(') {
                groupExpression(childPair);
            } else {
                factor(childPair);
            }
            //头尾相连即可
            pair.end->next1 = childPair.start;
            pair.end->edgeType = NFAEdgeType::epslion;
            pair.end = childPair.end;
        }
    }

    // expression ::= factorConnect ("|" factorConnect)*
    void NFA::expression(NFANodePair &pair) {
        factorConnect(pair);
        NFANodePair childPair;
        while (lexer.match(RegExToken::Or)) {
            lexer.advance();
            factorConnect(childPair);

            std::shared_ptr<NFANode> start = std::make_shared<NFANode>(NFAEdgeType::epslion);
            start->next1 = childPair.start;
            start->next2 = pair.start;

            std::shared_ptr<NFANode> end = std::make_shared<NFANode>();
            childPair.end->next1 = end;
            childPair.end->edgeType = NFAEdgeType::epslion;
            pair.end->next1 = end;
            pair.end->edgeType = NFAEdgeType::epslion;

            pair.start = start;
            pair.end = end;
        }
    }

    // groupExpression ::= ("(" expression ")")*
    void NFA::groupExpression(NFANodePair &pair) {
        if (lexer.match(RegExToken::LeftParen)) {
            lexer.advance();
            expression(pair);
            if (lexer.match(RegExToken::RightParen))
                lexer.advance();
        } else if (lexer.match(RegExToken::Eof)) {
            return;
        } else {
            expression(pair);
        }
        //解析?,*,+
        char c = lexer.getCurrentChar();
        switch (c) {
        case '?':
            questionClosure(pair);
            break;
        case '+':
            positiveClosure(pair);
            break;
        case '*':
            kleeneClosure(pair);
            break;
        default:
            lexer.advance();
            break;
        }
    }

    // regexExpression ::= ("(" groupExpression ")")*
    void NFA::regexExpression(NFANodePair &pair) {
        if (lexer.match(RegExToken::LeftParen)) {
            lexer.advance();
            groupExpression(pair);
            if (lexer.match(RegExToken::RightParen))
                lexer.advance();
        } else if (lexer.match(RegExToken::Eof)) {
            return;
        } else {
            groupExpression(pair);
        }
        //解析多个(),用·连接即可
        while (true) {
            NFANodePair childPair;
            if (lexer.match(RegExToken::LeftParen)) {
                lexer.advance();
                groupExpression(childPair);
                pair.end->next1 = childPair.start;
                pair.end->edgeType = NFAEdgeType::epslion;
                pair.end = childPair.end;

                if (lexer.match(RegExToken::RightParen))
                    lexer.advance();
            } else if (lexer.match(RegExToken::Eof)) {
                return;
            } else {
                groupExpression(childPair);
                pair.end->next1 = childPair.start;
                pair.end->edgeType = NFAEdgeType::epslion;
                pair.end = childPair.end;
            }
        }
    }

    // closure算法
    hashSet<NFANode *> NFA::closure(hashSet<NFANode *> &closureSet) {
        if (closureSet.empty())
            return closureSet;
        // 把传入集合里的所有节点压入栈中
        // 然后对这个栈的所有节点进行判断是否有可以直接跳转的节点
        // 如果有的话直接压入栈中
        // 直到栈为空则结束操作
        std::stack<NFANode *> nodeStack;
        for (NFANode *node : closureSet)
            nodeStack.push(node);

        while (!nodeStack.empty()) {
            NFANode *node = nodeStack.top();
            nodeStack.pop();
            NFANode *next1 = node->next1.get();
            if (next1 != nullptr && node->edgeType == NFAEdgeType::epslion) {
                if (closureSet.find(next1) == closureSet.end()) {
                    closureSet.emplace(next1);
                    nodeStack.push(next1);
                }
            }

            NFANode *next2 = node->next2.get();
            if (next2 != nullptr && node->edgeType == NFAEdgeType::epslion) {
                if (closureSet.find(next2) == closureSet.end()) {
                    closureSet.emplace(next2);
                    nodeStack.push(next2);
                }
            }
            //大多数时候loopShared都为空
            std::shared_ptr<NFANode> loopShared = node->loop.lock();
            if (loopShared != nullptr) {
                NFANode *loop = loopShared.get();
                if (loop != nullptr && node->edgeType == NFAEdgeType::epslion) {
                    if (closureSet.find(loop) == closureSet.end()) {
                        closureSet.emplace(loop);
                        nodeStack.push(loop);
                    }
                }
            }
        }
        return closureSet;
    }

    // DFAedge算法
    hashSet<NFANode *> NFA::DFAedge(hashSet<NFANode *> &closureSet, char c) {
        // DFAedge(s,c)为s集合中所有状态经过c能到到的集合
        hashSet<NFANode *> nextSet;
        for (NFANode *node : closureSet) {
            if (node->edgeType == NFAEdgeType::normalChar && node->edgeValue == c) {
                //单字符时
                nextSet.emplace(node->next1.get());
            } else if (node->edgeType == NFAEdgeType::charCollection) {
                //字符集时
                if (node->edgeValue == '.') {
                    //匹配anyChar
                    nextSet.emplace(node->next1.get());
                } else {
                    std::shared_ptr<hashSet<char>> &nodeChild = node->edgeSet;
                    for (char childChar : *nodeChild) {
                        //如果其子集中存在char c,则可访问
                        if (childChar == c) {
                            nextSet.emplace(node->next1.get());
                            break;
                        }
                    }
                }
            }
        }
        //最后还得求一次闭包
        closure(nextSet);
        return nextSet;
    }

    //获取DFA
    DFA NFA::NFAToDFA() {
        hashSet<NFANode *> currentStatus;
        currentStatus.emplace(head.get());
        currentStatus = NFA::closure(currentStatus);

        DFA dfa;
        // closureMap用于记录closure集合是否重复
        // key为closure集合,value为其对应的编号
        hashMap<hashSet<NFANode *>, int, DFAMapHash, DFAMapEqual> closureMap;
        std::vector<hashSet<NFANode *>> closureList;
        closureMap[currentStatus] = (int)closureMap.size();
        closureList.emplace_back(currentStatus);
        dfa.statusMap.emplace_back(DFA::isFinalStatus(currentStatus));
        for (int index = 0; index < closureMap.size(); index++) {
            for (int c = CHAR_MIN; c <= CHAR_MAX; c++) {
                hashSet<NFANode *> nfaClosure = NFA::DFAedge(closureList[index], (char)c);
                if (nfaClosure.empty()) {
                    continue;
                }
                // nfaClosure不存在,则需要加入新状态
                if (closureMap.find(nfaClosure) == closureMap.end()) {
                    closureMap[nfaClosure] = (int)closureMap.size();
                    closureList.emplace_back(nfaClosure);
                    dfa.statusMap.emplace_back(DFA::isFinalStatus(nfaClosure));
                }
                //这行表示,从closureList[index]状态经过c将转移到nfaClosure状态上
                int currentStatusNumber = closureMap[closureList[index]];
                while (dfa.table.size() <= currentStatusNumber) {
                    //每次currentStatusNumber不存在则其对应的size应该刚好等于currentStatusNumber
                    //因为statusNumber是一个个增加的
                    dfa.table.emplace_back(hashMap<char, int>());
                }
                //这行表示,从closureList[index]状态经过c将转移到nfaClosure状态上
                dfa.table[closureMap[closureList[index]]][(char)c] = closureMap[nfaClosure];
            }
        }
        //如果最后statusMap.size() > table.size(),则说明终态没有表现出来
        if (dfa.statusMap.size() > dfa.table.size()) {
            dfa.table.emplace_back(hashMap<char, int>());
        }
        return dfa;
    }

    //整个input字符串是否匹配pattern
    bool NFA::match(std::string_view &input) {
        hashSet<NFANode *> closureSet;
        closureSet.emplace(head.get());
        //输入空字时可达到的节点称为closure闭包
        closure(closureSet);
        // 首先先计算出开始节点的closure集合开始遍历输入的字符串
        // 从刚刚的closure集合开始做move操作然后判断当前的集合是不是可以作为接收状态
        // 只要当前集合有某个状态节点没有连接到其它节点，它就是一个可接收的状态节点
        // 能被当前NFA接收还需要一个条件就是当前字符已经全匹配完了
        int len = (int)input.size();
        for (int i = 0; i < len; i++) {
            closureSet = DFAedge(closureSet, input[i]);
            if (closureSet.empty())
                return false;
        }
        //最后查看closureSet中是否存在终结点
        for (NFANode *nfaNode : closureSet) {
            if (nfaNode->edgeType == NFAEdgeType::eofEdge)
                return true;
        }
        return false;
    }

    //找出所有匹配的string
    std::vector<std::string_view> NFA::contains(std::string_view &input) {
        std::vector<std::string_view> ans;
        int len = input.size();
        //初始状态的闭包
        hashSet<NFANode *> startSet;
        startSet.emplace(head.get());
        closure(startSet);
        //用于转换的set
        hashSet<NFANode *> closureSet = startSet;
        int index = 0;
        //然后通过input进行状态转移
        for (int i = 0; i < len; i++) {
            hashSet<NFANode *> nextClosureSet = DFAedge(closureSet, input[i]);
            if (nextClosureSet.empty()) {
                //说明转移失败,字符不匹配
                //判断当前closureSet中是否存在终结状态
                for (NFANode *node : closureSet) {
                    //存在终结态
                    if (node->edgeType == NFAEdgeType::eofEdge) {
                        std::string_view s = input.substr(index, i - index);
                        ans.emplace_back(s);
                        break;
                    }
                }
                //之后更新index并重置状态为初始状态
                index = i;
                closureSet = startSet;
            }
            nextClosureSet = DFAedge(closureSet, input[i]);
            if (!nextClosureSet.empty())
                closureSet = nextClosureSet;
            else
                index = i + 1;
        }
        //最末尾情况
        for (NFANode *node : closureSet) {
            //存在终结态
            if (node->edgeType == NFAEdgeType::eofEdge) {
                std::string_view s = input.substr(index, len - index + 1);
                ans.emplace_back(s);
                break;
            }
        }
        return ans;
    }
}  // namespace zhRegex