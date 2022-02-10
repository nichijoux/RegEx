#include "DFA.h"

namespace zhRegex {
    //构造函数
    DFA::DFA(const char *pattern, bool getMINDFA) {
        NFA machine(pattern);
        *this = machine.NFAToDFA();
        if (getMINDFA)
            getMinimizeDFA();
    }

    DFA::DFA(std::string &pattern, bool getMINDFA) {
        NFA machine(pattern);
        *this = machine.NFAToDFA();
        if (getMINDFA)
            getMinimizeDFA();
    }

    DFA::DFA(std::string_view &pattern, bool getMINDFA) {
        NFA machine(pattern);
        *this = machine.NFAToDFA();
        if (getMINDFA)
            getMinimizeDFA();
    }

    DFA::DFA(NFA &machine, bool getMINDFA) {
        *this = machine.NFAToDFA();
        if (getMINDFA)
            getMinimizeDFA();
    }

    //判断是否存在最终状态
    bool DFA::isFinalStatus(hashSet<NFANode *> &closureSet) {
        for (NFANode *nfaNode: closureSet) {
            if (nfaNode->edgeType == NFAEdgeType::eofEdge) {
                return true;
            }
        }
        return false;
    }

    //查看某个状态集合是否在现行已划分的集合中
    bool DFA::isStatusSetInExistSet(
            std::vector<hashSet<int>> &existStatusSet, hashSet<int> &nextStatusSet) {
        for (auto &set: existStatusSet) {
            bool flag = true;
            for (int status: nextStatusSet) {
                if (set.find(status) == set.end()) {
                    flag = false;
                    break;
                }
            }
            if (flag) {
                return true;
            }
        }
        return false;
    }

    //查看某个状态集合是否需要划分
    bool DFA::needSplit(std::vector<hashSet<int>> &completeStatusSet,
                        std::vector<hashSet<int>> &splitNotStatusSet, hashSet<int> &nextStatusSet) {
        if (nextStatusSet.size() == 1) {
            return false;
        }
        //先看completeStatusSet
        //后看splitNotStatusSet
        return !(isStatusSetInExistSet(completeStatusSet, nextStatusSet) ||
                 isStatusSetInExistSet(splitNotStatusSet, nextStatusSet));
    }

    //划分nextStatusSet
    void DFA::statusPartition(
            std::vector<hashSet<int>> &completeStatusSet,
            std::vector<hashSet<int>> &splitNotStatusSet, hashSet<int> &waitStatusSet, char c) {
        //遍历nextStatusSet并查看各个状态在其余状态中的编号
        std::vector<int> nextStatusList;
        hashMap<int, int> currentNextStatusMap;
        for (int status: waitStatusSet) {
            //如果不存在c边则表示留在原状态
            if (table[status].find(c) == table[status].end()) {
                currentNextStatusMap[status] = status;
            } else {
                currentNextStatusMap[status] = table[status][c];
            }
        }
        //对于nextStatusList,需要遍历completeStatusSet和splitNotStatusSet,找到他们各在那个集合中,用于划分
        int completeStatusSetSize = (int) completeStatusSet.size();
        int splitNotStatusSetSize = (int) splitNotStatusSet.size();
        for (auto &[k, v]: currentNextStatusMap) {
            bool flag = false;
            for (int i = 0; i < completeStatusSetSize; i++) {
                hashSet<int> &set = completeStatusSet[i];
                if (set.find(v) != set.end()) {
                    //找到nextStatus处在的集合,则令状态k为划分的i
                    currentNextStatusMap[k] = i;
                    flag = true;
                    break;
                }
            }
            if (flag)
                continue;
            for (int i = 0; i < splitNotStatusSetSize; i++) {
                hashSet<int> &set = splitNotStatusSet[i];
                if (set.find(v) != set.end()) {
                    //找到nextStatus处在的集合,则令状态k为划分的i + completeStatusSetSize
                    currentNextStatusMap[k] = i + completeStatusSetSize;
                    break;
                }
            }
        }
        //再遍历一次currentNextStatusMap划分状态
        hashMap<int, hashSet<int>> splitMap;
        for (auto &[k, v]: currentNextStatusMap) {
            splitMap[v].emplace(k);
        }
        splitNotStatusSet.pop_back();
        for (auto &[k, v]: splitMap) {
            splitNotStatusSet.emplace_back(v);
        }
    }

    //根据边划分
    void DFA::statusPartition(hashSet<int> &waitStatusSet, char c, std::vector<hashSet<int>> &splitNotStatusSet) {
        hashSet<int> containSet;
        hashSet<int> notContainSet;
        for (int status: waitStatusSet) {
            if (table[status].find(c) != table[status].end()) {
                containSet.emplace(status);
            } else {
                notContainSet.emplace(status);
            }
        }
        splitNotStatusSet.pop_back();
        splitNotStatusSet.emplace_back(containSet);
        splitNotStatusSet.emplace_back(notContainSet);
    }

    //获取最小DFA(Hopcroft算法)
    void DFA::getMinimizeDFA() {
        //已划分好的状态集
        std::vector<hashSet<int>> completeStatusSet;
        //待划分的状态集
        std::vector<hashSet<int>> splitNotStatusSet;
        //先划分终态和非终态
        hashSet<int> partfinalStatusSet;
        hashSet<int> finalStatusSet;
        hashSet<int> notFinalStatusSet;
        for (int i = 0; i < statusMap.size(); i++) {
            //对于不接收任何字符的终态,应该
            if (statusMap[i]) {
                finalStatusSet.emplace(i);
                if (table[i].empty()) {
                    hashSet<int> empty{i};
                    completeStatusSet.emplace_back(empty);
                    continue;
                }
                partfinalStatusSet.emplace(i);
            } else {
                notFinalStatusSet.emplace(i);
            }
        }
        splitNotStatusSet.emplace_back(notFinalStatusSet);
        splitNotStatusSet.emplace_back(partfinalStatusSet);
        //然后从待划分的状态集合中继续划分集合
        SPLIT:
        while (!splitNotStatusSet.empty()) {
            //从中选取一个待划分的集合
            hashSet<int> &waitStatusSet = splitNotStatusSet[splitNotStatusSet.size() - 1];
            //只剩一个状态时无需划分
            if (waitStatusSet.size() == 1) {
                completeStatusSet.emplace_back(waitStatusSet);
                splitNotStatusSet.pop_back();
                continue;
            }
            bool splitFlag = false;
            //先看waitStatusSet中的状态是否可通过边区分()
            for (int i: waitStatusSet) {
                for (int j: waitStatusSet) {
                    for (auto &[c, s]: table[i]) {
                        if (table[j].find(c) == table[j].end()) {
                            //说明需要至少需要通过c来划分
                            statusPartition(waitStatusSet, c, splitNotStatusSet);
                            goto SPLIT;
                        }
                    }
                }
            }
            for (int c = CHAR_MIN; c <= CHAR_MAX; c++) {
                // waitStatus经过c后会到达的集合即为nextStatusSet
                hashSet<int> nextStatusSet;
                for (int status: waitStatusSet) {
                    if (table[status].find((char) c) == table[status].end()) {
                        nextStatusSet.emplace(status);
                    } else {
                        nextStatusSet.emplace(table[status][(char) c]);
                    }
                }
                //然后查看nextStatus是否在某个现行划分的集合中,如果需要划分则
                if (needSplit(completeStatusSet, splitNotStatusSet, nextStatusSet)) {
                    //调用statusPartition函数划分waitStatusSet
                    statusPartition(completeStatusSet,
                                    splitNotStatusSet, waitStatusSet, (char) c);
                    splitFlag = true;
                    break;
                }
            }
            //如果遍历后发现不需要划分,则
            if (!splitFlag) {
                completeStatusSet.emplace_back(waitStatusSet);
                splitNotStatusSet.pop_back();
            }
        }
        //根据新的集合创建table
        //原table状态status对应的现在的状态
        int len = (int) completeStatusSet.size();
        hashMap<int, int> tableStatusMap;
        for (int i = 0; i < len; i++) {
            hashSet<int> &statusSet = completeStatusSet[i];
            //每个状态应该转到哪一个状态去
            for (int status: statusSet) {
                tableStatusMap[status] = i;
            }
        }
        //重构table后起始点会改变
        std::vector<hashMap<char, int>> newTable(len, hashMap<char, int>(CHAR_MAX - CHAR_MIN + 1));
        std::vector<bool> newStatusMap(len, false);
        //新的table应该从各个状态集合中选取代表
        for (int i = 0; i < len; i++) {
            hashSet<int> &statusSet = completeStatusSet[i];
            //每个状态应该转到哪一个状态去
            for (int status: statusSet) {
                for (int c = CHAR_MIN; c <= CHAR_MAX; c++) {
                    //原table状态转到的status对应的现在的map
                    if (table[status].find((char) c) != table[status].end()) {
                        int nextStatus = table[status][(char) c];
                        newTable[i][(char) c] = tableStatusMap[nextStatus];
                    }
                }
                //为终态则令其为true
                if (finalStatusSet.find(status) != finalStatusSet.end()) {
                    newStatusMap[i] = true;
                }
            }
        }
        int start = tableStatusMap[0];
        this->startNode = start;
        this->table = newTable;
        this->statusMap = newStatusMap;
    }

    //整个input字符串是否匹配pattern
    bool DFA::match(std::string_view &input) {
        int status = startNode;
        for (char c: input) {
            if (table[status].find(c) == table[status].end()) {
                return false;
            }
            status = table[status][c];
        }
        return statusMap[status];
    }

    //找出所有匹配的string
    std::vector<std::string_view> DFA::contains(std::string_view &input) {
        int status = startNode;
        int len = (int) input.size();
        std::vector<std::string_view> ans;
        int index = 0;
        for (int i = 0; i < len; i++) {
            //当发现不匹配时
            if (table[status].find(input[i]) == table[status].end()) {
                //如果当前状态可作为终结状态,则插入
                if (statusMap[status]) {
                    //大小应该从index出发截止到i - 1的位置
                    std::string_view s = input.substr(index, i - index);
                    ans.emplace_back(s);
                }
                //之后更新index并重置状态为初始状态
                // index应该从当前这个不匹配的字符开始算起
                index = i;
                status = startNode;
            }
            if (table[status].find(input[i]) != table[status].end()) {
                status = table[status][input[i]];
            } else {
                index = i + 1;
            }
        }
        if (statusMap[status]) {
            std::string_view s = input.substr(index, len - index + 1);
            ans.emplace_back(s);
        }
        return ans;
    }
}  // namespace zhRegex