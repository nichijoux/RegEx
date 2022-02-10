#ifndef _ZH_DFA_H_
#define _ZH_DFA_H_

#include "NFA.h"

namespace zhRegex {
    // MapHash和MapEqual
    struct DFAMapHash {
        size_t operator()(const hashSet<NFANode *> &set) const {
            size_t ans = 0;
            for (NFANode *node : set) {
                ans ^= std::hash<char>()(node->edgeValue);
                ans ^= std::hash<int>()(static_cast<int>(node->edgeType));
                if (node->edgeSet != nullptr) {
                    for (char c : *(node->edgeSet)) {
                        ans ^= std::hash<char>()(c);
                    }
                }
            }
            return ans;
        }
    };

    struct DFAMapEqual {
        bool operator()(const hashSet<NFANode *> &set1, const hashSet<NFANode *> &set2) const noexcept {
            if (set1.size() != set2.size()) {
                return false;
            }
            return set1 == set2;
        }
    };

    //确定有限状态机
    class DFA : public Pattern {
    private:
        // table表用于状态转换
        std::vector<hashMap<char, int>> table;
        // statusMap用于指示否个状态是否为最终状态
        std::vector<bool> statusMap;
        //起始点
        int startNode{0};
        //友元
        friend class NFA;

    private:
        //判断是否存在最终状态
        static bool isFinalStatus(hashSet<NFANode *> &closureSet);

        //查看某个状态集合是否在现行已划分的集合中
        static bool isStatusSetInExistSet(std::vector<hashSet<int>> &existStatusSet,
                                          hashSet<int> &nextStatusSet);

        //查看某个状态集合是否需要划分
        static bool needSplit(std::vector<hashSet<int>> &completeStatusSet,
                              std::vector<hashSet<int>> &splitNotStatusSet, hashSet<int> &nextStatusSet);

        //划分waitStatusSet
        void statusPartition(std::vector<hashSet<int>> &completeStatusSet,
                             std::vector<hashSet<int>> &splitNotStatusSet, hashSet<int> &waitStatusSet, char c);

        //根据边划分
        void statusPartition(hashSet<int> &waitStatusSet, char c, std::vector<hashSet<int>> &splitNotStatusSet);
        //获取最小DFA(Hopcroft算法)
        void getMinimizeDFA();

        DFA() = default;

    public:
        explicit DFA(const char *pattern, bool getMINDFA = true);
        explicit DFA(std::string &pattern, bool getMINDFA = true);
        explicit DFA(std::string_view &pattern, bool getMINDFA = true);
        explicit DFA(NFA &nfaMachine, bool getMINDFA = true);
        ~DFA() override = default;

        //整个input字符串是否匹配pattern
        bool match(std::string_view &input) override;

        //找出所有匹配的string
        std::vector<std::string_view> contains(std::string_view &input) override;
    };
}  // namespace zhRegex

#endif  // !_DFA_H_
