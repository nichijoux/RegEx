#include <iostream>

#include "Regex.h"

using namespace std;
using namespace zhRegex;

int main(int argc, char *argv[]) {
    cout << "Hello, world!" << endl;
    // string input = "THISISREGEXTEST";
    // string pattern = "([A-Z]*|[0-9]+)";
    string_view input = "-127.27e+126f";
    string_view pattern = "[+-]?[0-9]+(\\.[0-9]+)?(e[+-]?[0-9]+)?f?";
    // const char *input = "12641318sdu3j31bd913kj11s";
    // const char *pattern = "\\d\\w+";
    Pattern *machine = new DFA(pattern);
    Regex regex(machine);
    cout << regex.match(input) << "\n";
    auto ans = regex.contains(input);
    for (auto &s: ans) {
        cout << s << endl;
    }
    delete machine;
    return 0;
}