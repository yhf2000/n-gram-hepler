/*
 *
*/
#include <vector>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <queue>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

class SuffixArray {
public:
    struct node{
        int id, cnt;
        bool operator<(const node& other) const {
            return cnt > other.cnt;
        }
    };

    const int L_TYPE = 0;
    const int S_TYPE = 1;
    int SA_length;
    int *S;
    int *result_SA;
    int *result_RK;

    ~SuffixArray() {
        delete[] S;
        delete[] result_SA;
        delete[] result_RK;
    }

    bool is_lms_char(const int *type, int x) {
        return x > 0 && type[x] == S_TYPE && type[x - 1] == L_TYPE;
    }

    bool equal_substring(const int *S, int x, int y, const int *type) {
        do {
            if (S[x] != S[y])
                return false;
            x++, y++;
        } while (!is_lms_char(type, x) && !is_lms_char(type, y));
        return S[x] == S[y];
    }

    void induced_sort(int *S, int *SA, int *type, int *bucket, int *lbucket, int *sbucket, int n, int SIGMA) {
        for (int i = 0; i <= n; i++)
            if (SA[i] > 0 && type[SA[i] - 1] == L_TYPE)
                SA[lbucket[S[SA[i] - 1]]++] = SA[i] - 1;
        for (int i = 1; i <= SIGMA; i++)
            sbucket[i] = bucket[i] - 1;
        for (int i = n; i >= 0; i--)
            if (SA[i] > 0 && type[SA[i] - 1] == S_TYPE)
                SA[sbucket[S[SA[i] - 1]]--] = SA[i] - 1;
    }

    int *SAIS(int *S, int length, int SIGMA) {
        int n = length - 1;
        int *type = new int[n + 1];  // 后缀类型
        int *position = new int[n + 1];  // 记录LMS子串的起始位置
        int *name = new int[n + 1];  // 记录每个LMS子串的新名称
        int *SA = new int[n + 1];  // SA数组
        int *bucket = new int[SIGMA + 1];  // 每个字符的桶
        int *lbucket = new int[SIGMA + 1];  // 每个字符的L型桶的起始位置
        int *sbucket = new int[SIGMA + 1];  // 每个字符的S型桶的起始位置

        std::fill(bucket, bucket + SIGMA + 1, 0);
        for (int i = 0; i <= n; i++)
            bucket[S[i]]++;
        lbucket[0] = sbucket[0] = 0;
        for (int i = 1; i <= SIGMA; i++) {
            bucket[i] += bucket[i - 1];
            lbucket[i] = bucket[i - 1];
            sbucket[i] = bucket[i] - 1;
        }

        type[n] = S_TYPE;
        for (int i = n - 1; i >= 0; i--) {
            if (S[i] < S[i + 1])
                type[i] = S_TYPE;
            else if (S[i] > S[i + 1])
                type[i] = L_TYPE;
            else
                type[i] = type[i + 1];
        }

        // 寻找每个LMS子串
        int cnt = 0;
        for (int i = 1; i <= n; i++)
            if (type[i] == S_TYPE && type[i - 1] == L_TYPE)
                position[cnt++] = i;



        std::fill(SA, SA + n + 1, -1);
        for (int i = 0; i < cnt; i++)
            SA[sbucket[S[position[i]]]--] = position[i];
        induced_sort(S, SA, type, bucket, lbucket, sbucket, n, SIGMA);

        std::fill(name, name + n + 1, -1);
        int lastx = -1, namecnt = 1;  // 上一次处理的LMS子串与名称的计数
        bool flag = false;  // 这里顺便记录是否有重复的字符
        for (int i = 1; i <= n; i++) {
            int x = SA[i];

            if (is_lms_char(type, x)) {
                if (lastx >= 0 && !equal_substring(S, x, lastx, type))
                    namecnt++;
                // 因为只有相同的LMS子串才会有同样的名称
                if (lastx >= 0 && namecnt == name[lastx])
                    flag = true;

                name[x] = namecnt;
                lastx = x;
            }
        }  // for
        name[n] = 0;

        // 生成S1
        int *S1 = new int[cnt];
        int pos = 0;
        for (int i = 0; i <= n; i++)
            if (name[i] >= 0)
                S1[pos++] = name[i];

        int *SA1;
        if (!flag) {
            // 直接计算SA1
            SA1 = new int[cnt + 1];

            for (int i = 0; i < cnt; i++)
                SA1[S1[i]] = i;
        } else
            SA1 = SAIS(S1, cnt, namecnt);  // 递归计算SA1

        // 从SA1诱导到SA
        lbucket[0] = sbucket[0] = 0;
        for (int i = 1; i <= SIGMA; i++) {
            lbucket[i] = bucket[i - 1];
            sbucket[i] = bucket[i] - 1;
        }
        std::fill(SA, SA + n + 1, -1);
        for (int i = cnt - 1; i >= 0; i--)  // 这里是逆序扫描SA1，因为SA中S型桶是倒序的
            SA[sbucket[S[position[SA1[i]]]]--] = position[SA1[i]];
        induced_sort(S, SA, type, bucket, lbucket, sbucket, n, SIGMA);


        delete[] S1;
        delete[] SA1;
        delete[] type;
        delete[] position;
        delete[] name;
        delete[] bucket;
        delete[] lbucket;
        delete[] sbucket;
        return SA;
    }

    int compare(int pos, const std::vector<int>& pattern) {
        int len = pattern.size();
        for (int i = 0; i < len; i++) {
            if (pos + i >= SA_length) return 1; // 模式更长
            if (S[pos + i] < pattern[i]) return -1;
            if (S[pos + i] > pattern[i]) return 1;
        }
        return 0;
    }
    void print()
    {
        for (int i = 0; i < SA_length; i++) std::cout << result_SA[i] << ' '; std::cout << std::endl;
        for (int i = 0; i < SA_length; i++) std::cout << result_RK[i] << ' '; std::cout << std::endl;
    }

    void build(const std::vector<int>& s, int SIGMA) {
        int length = s.size();
        int *new_s = new int[length + 2];
        for (int i = 0; i < length; i++)
            new_s[i] = s[i] + 10;
        new_s[length] = 1;

        int *SA = SAIS(new_s, length + 1, SIGMA);

        S = new int[length];
        result_SA = new int[length];
        result_RK = new int[length];
        for (int i = 0; i < length; i++)
            result_SA[i] = SA[i + 1];
        for (int i = 0; i < length; i++)
            result_RK[result_SA[i]] = i;
        for (int i = 0; i < length; i++)
            S[i] = s[i];
        SA_length = length;
        // print();
    }

    std::pair<std::vector<int>, int> query(const std::vector<int>& pattern) {
        std::vector<int> positions;
        int l = 0, r = SA_length-1;

        // 二分查找左边界
        while (l <= r) {
            int mid = (l + r) / 2;
            int cmp = compare(result_SA[mid], pattern);
            if (cmp >= 0) r = mid - 1;
            else l = mid + 1;
        }
        int left = l;

        // 二分查找右边界
        l = 0, r = SA_length-1;
        while (l <= r) {
            int mid = (l + r) / 2;
            int cmp = compare(result_SA[mid], pattern);
            if (cmp <= 0) l = mid + 1;
            else r = mid - 1;
        }
        int right = r;

        // 收集结果
        for (int i = left; i <= right; i++) {
            positions.push_back(result_SA[i] + pattern.size());
        }
        return {positions, right - left + 1};
    }

    void read(std::string path)
    {
        std::vector<int> numbers;
        std::ifstream file(path);
        if (!file.is_open())  return;
        std::string line_str;
        while (std::getline(file, line_str)) {
            // line_str.erase(std::remove(line_str.begin(), line_str.end(), '['), line_str.end());
            // line_str.erase(std::remove(line_str.begin(), line_str.end(), ']'), line_str.end());
            line_str.erase(line_str.begin());
            // line_str.erase(line_str.end());
            line_str.pop_back();
            std::replace(line_str.begin(), line_str.end(), ',', ' ');

            std::istringstream iss(line_str);
            int num;
            while (iss >> num) {
                numbers.push_back(num);
            }
            numbers.push_back(128001);
        }
        build(numbers, 129000);
    }

    std::pair<std::vector<int>, std::vector<float>> find(std::vector<int> tokens, int top) {
        std::pair<std::vector<int>, int> result = query(tokens);
        float num = 0;
        std::vector<int> positions = result.first;
        std::vector<node> members;
        std::unordered_map <int, int> vis;
        for (auto i : positions) {
            if (i < SA_length) {
                int token = S[i];
                vis[token]++;
                num++;
            }
        }
        for (auto i : positions) {
            if (i < SA_length) {
                int token = S[i];
                members.push_back({token, vis[token]});
            }
        }
        sort(members.begin(), members.end());
        std::pair<std::vector<int>, std::vector<float>> R;
        for (int i = 0; i < top && i < members.size(); i++)
        {
            R.first.push_back(members[i].id);
            R.second.push_back(vis[members[i].id] / num);
        }
        return R;
    }
};

SuffixArray sa_chat, sa_stack;

std::pair<std::vector<std::vector<int>>, std::vector<std::vector<float>>> chat_get_SA_tokens(std::vector<std::vector<int>> input, int top) {
    std::pair<std::vector<std::vector<int>>, std::vector<std::vector<float>>> results;
    for (auto tokens : input) {

        for (int i = 0; i < tokens.size(); i++)
        {
            std::vector<int> T;
            for (auto j = tokens.begin() + i; j != tokens.end(); j++)  T.push_back(*j);
            std::pair<std::vector<int>, std::vector<float>> result = sa_chat.find(T, top);

            if (!result.first.empty())
            {
                results.first.push_back(result.first);
                results.second.push_back(result.second);
                break;
            }
        }
    }
    return results;
}

std::pair<std::vector<std::vector<int>>, std::vector<std::vector<float>>> stack_get_SA_tokens(std::vector<std::vector<int>> input, int top) {
    std::pair<std::vector<std::vector<int>>, std::vector<std::vector<float>>> results;
    for (auto tokens : input) {

        for (int i = 0; i < tokens.size(); i++)
        {
            std::vector<int> T;
            for (auto j = tokens.begin() + i; j != tokens.end(); j++)  T.push_back(*j);
            std::pair<std::vector<int>, std::vector<float>> result = sa_stack.find(T, top);

            if (!result.first.empty())
            {
                results.first.push_back(result.first);
                results.second.push_back(result.second);
                break;
            }
        }
    }
    return results;
}

void chatbuild(std::string path)
{
    sa_chat.read(path);
}

void stackbuild(std::string path)
{
    sa_stack.read(path);
}

PYBIND11_MODULE(SA_token, m) {
    m.def("chatbuild", &chatbuild, "input txt path, Load data and build SA");
    m.def("stackbuild", &stackbuild, "input txt path, Load data and build SA");
    m.def("chat_get_SA_tokens", &chat_get_SA_tokens, "input&output vector<int>, Query suffix tokens");
    m.def("stack_get_SA_tokens", &stack_get_SA_tokens, "input&output vector<int>, Query suffix tokens");
}

int main()
{
    // sa.build({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 1, 2}, 130000);
    // std::pair<std::vector<std::vector<int>>, std::vector<std::vector<float>>>ans = get_SA_tokens({{1, 2}}, 2);
    // for (auto i : ans.first)
    //     for (auto out : i)
    //         std::cout << out << ' ';
    // std::cout << std::endl;
    // for (auto i : ans.second)
    //     for (auto out : i)
    //         std::cout << out << ' ';
    // sa.read("/mnt/data1/lth/proc-ultrachat/datastore_chat_small.txt");

}