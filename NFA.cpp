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

    //????????????,?????????????????????
    void NFA::singleChar(NFANodePair &pair) {
        pair.start = std::make_shared<NFANode>(NFAEdgeType::normalChar);
        pair.end = std::make_shared<NFANode>();
        pair.start->next1 = pair.end;
        pair.start->edgeValue = lexer.getCurrentChar();

        lexer.advance();
    }

    //.??????
    void NFA::anyChar(NFANodePair &pair) {
        if (!lexer.match(RegExToken::AnyChar))
            return;
        pair.start = std::make_shared<NFANode>(NFAEdgeType::charCollection);
        pair.start->edgeValue = '.';
        pair.end = std::make_shared<NFANode>();
        pair.start->next1 = pair.end;
        lexer.advance();
    }

    //?????????
    void NFA::charCollection(NFANodePair &pair) {
        //?????????????????????currentToken?????????[
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
            //?????????????????????
            if (lexer.match(RegExToken::Dash)) {
                //?????????????????????????????????????????????,???????????????????????????-??????
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

    //????????????
    void NFA::escapeChar(NFANodePair &pair) {
        if (!lexer.match(RegExToken::EscapeChar))
            return;
        static char escapeCharacter[] = {
            'd',  //????????????
            'D',  //???????????????
            'w',  //????????????
            'W'   //???????????????
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

    //*??????
    void NFA::kleeneClosure(NFANodePair &pair) {
        std::shared_ptr<NFANode> start = std::make_shared<NFANode>(NFAEdgeType::epslion);
        std::shared_ptr<NFANode> end = std::make_shared<NFANode>();
        start->next1 = pair.start;
        start->next2 = end;

        //??????*?????????????????????,?????????????????????????????????loop??????
        pair.end->loop = pair.start;
        pair.end->next1 = end;
        pair.end->edgeType = NFAEdgeType::epslion;

        pair.start = start;
        pair.end = end;

        lexer.advance();
    }

    //+??????
    void NFA::positiveClosure(NFANodePair &pair) {
        std::shared_ptr<NFANode> start = std::make_shared<NFANode>(NFAEdgeType::epslion);
        std::shared_ptr<NFANode> end = std::make_shared<NFANode>();
        start->next1 = pair.start;

        //??????+?????????????????????,?????????????????????????????????loop??????
        pair.end->loop = pair.start;
        pair.end->next1 = end;
        pair.end->edgeType = NFAEdgeType::epslion;

        pair.start = start;
        pair.end = end;
        lexer.advance();
    }

    //???????
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

    //{n,m}??????
    void NFA::repeatClosure(NFANodePair &pair) {
        if (!lexer.match(RegExToken::LeftBrace))
            return;
        // n,m??????
        int n = 0;
        //??????n
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
        //???????????????????????????????????????
        if (lexer.match(RegExToken::RightBrace)) {
            //???????????????????????????,??????????????????{n}??????
            repeatClosureHelper(pair, n, -2);
        } else {
            //?????????????????????lexer.getCurrentChar()???????????????
            int m = 0;
            lexer.advance();
            if (lexer.match(RegExToken::RightBrace)) {
                //???????????? m ???????????????
                repeatClosureHelper(pair, n, -1);
            } else {
                //??????????????????m
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

    //{n,m}??????????????????,m = -2??????????????????,m = -1???????????????
    void NFA::repeatClosureHelper(NFANodePair &pair, int n, int m) {
        if (m > 0 && n > m) {
            throw RegexException();
        }
        if (n == 0 && m == -1) {
            //??????*??????
            kleeneClosure(pair);
        } else if (n == 1 && m == -1) {
            //??????+??????
            positiveClosure(pair);
        } else if (n == 0 && m == 1) {
            //?????????????
            questionClosure(pair);
        } else {
            if (n == 0 && m == 0) {
                pair.start = nullptr;
                pair.end = nullptr;
                return;
            }
            //{n,m}??????
            //??????start
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

                //?????????term????????????term??????
                pair.end->next1 = newStart;
                pair.end->edgeType = NFAEdgeType::epslion;

                //??????pair
                if (i == n - 1) {
                    prevTail = pair.end;
                }
                pair.start = newStart;
                pair.end = newEnd;
            }
            //{n,m}??????(n??????????????????,m???-2???-1???????????????????????????)
            for (int i = n; i < m; i++) {
                std::shared_ptr<NFANode> newStart = std::make_shared<NFANode>(type);
                newStart->edgeType = type;
                newStart->edgeValue = pair.start->edgeValue;
                newStart->edgeSet = pair.start->edgeSet;

                std::shared_ptr<NFANode> newEnd = std::make_shared<NFANode>(NFAEdgeType::epslion);
                newStart->next1 = newEnd;

                // epslion???
                std::shared_ptr<NFANode> newHead = std::make_shared<NFANode>(NFAEdgeType::epslion);
                std::shared_ptr<NFANode> newTail = std::make_shared<NFANode>();

                newHead->next1 = newStart;
                newHead->next2 = newTail;
                newEnd->next1 = newTail;

                //?????????term??????term
                pair.end->edgeType = NFAEdgeType::epslion;
                pair.end->next1 = newHead;

                //??????pair
                pair.end = newTail;
            }
            if (m == -1) {
                pair.end->edgeType = NFAEdgeType::epslion;
                pair.end->loop = prevTail;
                //?????????????????????tail
                std::shared_ptr<NFANode> tail = std::make_shared<NFANode>();
                pair.end->next1 = tail;
                pair.end = tail;
            }
            //????????????tail???head
            pair.start = saveHead;
        }
    }

    // factor ::= (("(")("^")term("$")(")") | ("(")("^")term("*" | "+" | "?" | "{n,m}")($)(")"))*
    void NFA::factor(NFANodePair &pair) {
        try {
            //???????????????????????????
            if (lexer.match(RegExToken::LeftParen))
                lexer.advance();
            //^??????
            if (lexer.match(RegExToken::CharBegin))
                lexer.advance();

            term(pair);
            //?????????
            switch (lexer.getCurrentToken()) {
            case RegExToken::Kleene:
                //*??????
                kleeneClosure(pair);
                break;
            case RegExToken::Positive:
                //+??????
                positiveClosure(pair);
                break;
            case RegExToken::Question:
                //???????
                questionClosure(pair);
                break;
            case RegExToken::LeftBrace:
                //{n,m}??????
                repeatClosure(pair);
                break;
            default:
                break;
            }

            //?????????????????????
            if (lexer.match(RegExToken::RightParen))
                lexer.advance();
            //$??????
            if (lexer.match(RegExToken::CharEnd))
                lexer.advance();
        } catch (RegexException &e) {
            printf("%s", e.what());
            return;
        }
    }

    //????????????factor?????????
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

    // factorConnect ::= factor | factor??factor*
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
            //??????????????????
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
        //???????,*,+
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
        //????????????(),?????????????????
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

    // closure??????
    hashSet<NFANode *> NFA::closure(hashSet<NFANode *> &closureSet) {
        if (closureSet.empty())
            return closureSet;
        // ?????????????????????????????????????????????
        // ?????????????????????????????????????????????????????????????????????????????????
        // ?????????????????????????????????
        // ??????????????????????????????
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
            //???????????????loopShared?????????
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

    // DFAedge??????
    hashSet<NFANode *> NFA::DFAedge(hashSet<NFANode *> &closureSet, char c) {
        // DFAedge(s,c)???s???????????????????????????c??????????????????
        hashSet<NFANode *> nextSet;
        for (NFANode *node : closureSet) {
            if (node->edgeType == NFAEdgeType::normalChar && node->edgeValue == c) {
                //????????????
                nextSet.emplace(node->next1.get());
            } else if (node->edgeType == NFAEdgeType::charCollection) {
                //????????????
                if (node->edgeValue == '.') {
                    //??????anyChar
                    nextSet.emplace(node->next1.get());
                } else {
                    std::shared_ptr<hashSet<char>> &nodeChild = node->edgeSet;
                    for (char childChar : *nodeChild) {
                        //????????????????????????char c,????????????
                        if (childChar == c) {
                            nextSet.emplace(node->next1.get());
                            break;
                        }
                    }
                }
            }
        }
        //???????????????????????????
        closure(nextSet);
        return nextSet;
    }

    //??????DFA
    DFA NFA::NFAToDFA() {
        hashSet<NFANode *> currentStatus;
        currentStatus.emplace(head.get());
        currentStatus = NFA::closure(currentStatus);

        DFA dfa;
        // closureMap????????????closure??????????????????
        // key???closure??????,value?????????????????????
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
                // nfaClosure?????????,????????????????????????
                if (closureMap.find(nfaClosure) == closureMap.end()) {
                    closureMap[nfaClosure] = (int)closureMap.size();
                    closureList.emplace_back(nfaClosure);
                    dfa.statusMap.emplace_back(DFA::isFinalStatus(nfaClosure));
                }
                //????????????,???closureList[index]????????????c????????????nfaClosure?????????
                int currentStatusNumber = closureMap[closureList[index]];
                while (dfa.table.size() <= currentStatusNumber) {
                    //??????currentStatusNumber????????????????????????size??????????????????currentStatusNumber
                    //??????statusNumber?????????????????????
                    dfa.table.emplace_back(hashMap<char, int>());
                }
                //????????????,???closureList[index]????????????c????????????nfaClosure?????????
                dfa.table[closureMap[closureList[index]]][(char)c] = closureMap[nfaClosure];
            }
        }
        //????????????statusMap.size() > table.size(),?????????????????????????????????
        if (dfa.statusMap.size() > dfa.table.size()) {
            dfa.table.emplace_back(hashMap<char, int>());
        }
        return dfa;
    }

    //??????input?????????????????????pattern
    bool NFA::match(std::string_view &input) {
        hashSet<NFANode *> closureSet;
        closureSet.emplace(head.get());
        //???????????????????????????????????????closure??????
        closure(closureSet);
        // ?????????????????????????????????closure????????????????????????????????????
        // ????????????closure???????????????move??????????????????????????????????????????????????????????????????
        // ????????????????????????????????????????????????????????????????????????????????????????????????????????????
        // ????????????NFA??????????????????????????????????????????????????????????????????
        int len = (int)input.size();
        for (int i = 0; i < len; i++) {
            closureSet = DFAedge(closureSet, input[i]);
            if (closureSet.empty())
                return false;
        }
        //????????????closureSet????????????????????????
        for (NFANode *nfaNode : closureSet) {
            if (nfaNode->edgeType == NFAEdgeType::eofEdge)
                return true;
        }
        return false;
    }

    //?????????????????????string
    std::vector<std::string_view> NFA::contains(std::string_view &input) {
        std::vector<std::string_view> ans;
        int len = input.size();
        //?????????????????????
        hashSet<NFANode *> startSet;
        startSet.emplace(head.get());
        closure(startSet);
        //???????????????set
        hashSet<NFANode *> closureSet = startSet;
        int index = 0;
        //????????????input??????????????????
        for (int i = 0; i < len; i++) {
            hashSet<NFANode *> nextClosureSet = DFAedge(closureSet, input[i]);
            if (nextClosureSet.empty()) {
                //??????????????????,???????????????
                //????????????closureSet???????????????????????????
                for (NFANode *node : closureSet) {
                    //???????????????
                    if (node->edgeType == NFAEdgeType::eofEdge) {
                        std::string_view s = input.substr(index, i - index);
                        ans.emplace_back(s);
                        break;
                    }
                }
                //????????????index??????????????????????????????
                index = i;
                closureSet = startSet;
            }
            nextClosureSet = DFAedge(closureSet, input[i]);
            if (!nextClosureSet.empty())
                closureSet = nextClosureSet;
            else
                index = i + 1;
        }
        //???????????????
        for (NFANode *node : closureSet) {
            //???????????????
            if (node->edgeType == NFAEdgeType::eofEdge) {
                std::string_view s = input.substr(index, len - index + 1);
                ans.emplace_back(s);
                break;
            }
        }
        return ans;
    }
}  // namespace zhRegex