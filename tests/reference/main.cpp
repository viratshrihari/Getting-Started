#include <algorithm>
#include <iostream>
#include <vector>
#include <iterator>
#include <string>
using namespace std;

int main() {
    constexpr int op = @OP@;
    if (op == -1) { cout << "moo\n"; return 0; }
    if (op == 11) {
        long long value;
        cin >> value;
        for (;;) {
            cout << value << (value == 1 ? '\n' : ' ');
            if (value == 1) return 0;
            value = value % 2 ? value * 3 + 1 : value / 2;
        }
    }
    if (op == 12 || op == 15 || op == 16) {
        int count = 1;
        if (op == 16) { string line; getline(cin, line); count = stoi(line); }
        string chant((istreambuf_iterator<char>(cin)), istreambuf_iterator<char>());
        if (op == 16 && !chant.empty() && chant.back() == '\n') chant.pop_back();
        for (int i = 0; i < count; i++) cout << chant;
        if (op == 16 && count && !chant.empty()) cout << '\n';
        return 0;
    }
    if (op == 13 || op == 14) {
        int count = 2;
        if (op == 14) cin >> count;
        vector<string> names(count);
        for (auto &name : names) cin >> name;
        if (op == 13) cout << (names[0].size() == names[1].size() ? "-1" : names[0].size() > names[1].size() ? "nohj" : "john") << '\n';
        else {
            auto longest = max_element(names.begin(), names.end(), [](const string &a, const string &b) { return a.size() < b.size(); });
            cout << longest->size() << '\n' << *longest << '\n';
        }
        return 0;
    }
    int n = op == 0 ? 1 : 2;
    if (op == 2 || op == 4 || op == 6 || op == 8 || op == 10) cin >> n;
    vector<long long> a(n);
    for (auto &value : a) cin >> value;
    if (op == 1 || op == 2) {
        reverse(a.begin(), a.end());
        for (auto value : a) cout << value << ' ';
        cout << '\n';
    } else {
        long long answer = a[0];
        for (int i = 1; i < n; i++) {
            switch (op) {
                case 3: case 4: answer += a[i]; break;
                case 5: case 6: answer %= a[i]; break;
                case 7: case 8: answer = answer * a[i] % 1000000007; break;
                case 9: case 10: answer /= a[i]; break;
            }
        }
        cout << answer << '\n';
    }
}
