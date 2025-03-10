#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <map>
#include <deque>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

class Trie {
public:
    struct node{
        int id = 0, cnt = 0;
        bool operator<(const node& other) const {
            return cnt > other.cnt;
        }
    };


    struct TrieNode {
        int vis_count = 0;
        int deepth = 0;
        int nearest_time = 0;
        int val = 0;
        std::vector<int> task_id;
        TrieNode* father = nullptr;
        std::unordered_map<int, TrieNode*> children;
        bool operator<(const TrieNode& other) const {
            if (nearest_time != other.nearest_time)
                return nearest_time < other.nearest_time;
            else if (deepth != other.deepth)
                return deepth > other.deepth;
            else
                return val > other.val;
        }
    };


    int max_node = 0;
    int n_node = 0;
    int n_time = 0;
    int task_num = 0;
    int branch_length = 0;
    std::map<std::string, int> task_to_id;
    std::map<std::string, int> task_to_time;
    std::map<TrieNode*, std::map<int,int>> vis;
    std::map<int, int> task_start;
    std::vector<std::vector<TrieNode*>> task_ptr;
    TrieNode* root;
    std::set<TrieNode> del_node;
    void clear(TrieNode* node) {
        if (!node) return;
        for (auto& pair : node->children) {
            clear(pair.second);
        }
        n_node--;
        delete node;
    }

    Trie() : root(new TrieNode()), max_node(65536), n_node(0), branch_length(16), n_time(0) {}

    ~Trie() {
        clear(root);
    }

    void del()
    {
        while (n_node > max_node)
        {
            auto i = *(del_node.begin());
            while (!i.task_id.empty())
            {
                if (task_start[i.task_id.back()] == 0)
                    i.task_id.pop_back();
                else
                    break;
            }
            if (!i.task_id.empty()) break;
            n_node --;
            i.father->children[i.val] = nullptr;
            del_node.erase(i);
//            delete &i;
        }
    }

    TrieNode* insert(const std::deque<int>& tokens, TrieNode* R, int Time, int Task_id) {
        TrieNode* current = R;
        for (int token : tokens) {
            if (!current->children.count(token)) {
                current->children[token] = new TrieNode();
                current->children[token]->father = current;
                current->children[token]->val = token;
                current->children[token]->deepth = current->deepth + 1;
                current->children[token]->nearest_time = Time;
//                current->children[token]->vis_count++;
                n_node++;
                if (vis[current->children[token]][Task_id] == 0)
                {
                    current->children[token]->task_id.push_back(Task_id);
                    vis[current->children[token]][Task_id] = 1;
                }
                del_node.insert(*(current->children[token]));
            }
            current = current->children[token];
            if(vis[current][Task_id] == 0)
            {
                current->task_id.push_back(Task_id);
                vis[current][Task_id] = 1;
            }
            if (current->nearest_time != Time)
            {
                del_node.erase(*current);
                current->nearest_time = Time;
                del_node.insert(*current);
            }
            current->vis_count++;
        }
        return current;
    }
    void set(int Branch_length, int Max_node)
    {
        branch_length = Branch_length;
        max_node = Max_node;
    }

    std::pair<std::vector<int>, std::vector<float>> find(std::vector<int> tokens, int top) {
        n_time++;
        TrieNode* cur = root;
        std::pair<std::vector<int>, std::vector<float>> result;
        for (auto i : tokens)
        {
            if (cur->children.count(i))
                cur = cur->children[i];
            else
                return result;
        }
        std::vector<node> members;
        std::unordered_map <int, int> vis;
        float num = 0;
        cur = root;
        for (auto i : tokens)
        {
            del_node.erase(*cur);
            cur->nearest_time = n_time;
            del_node.insert(*cur);
            cur = cur->children[i];
        }

        for (auto i : cur->children)
        {
            int token = i.first;
            vis[token] += i.second->vis_count;
        }
        for (auto i : cur->children)
        {
            int token = i.first;
            members.push_back({token, vis[token]});
        }
        sort(members.begin(), members.end());
        for (int i = 0; i < top && i < members.size(); i++)
            num += vis[members[i].id];
        for (int i = 0; i < top && i < members.size(); i++)
        {
            result.first.push_back(members[i].id);
            result.second.push_back(vis[members[i].id] / num);
        }
        return result;
    }


    void INSERT(std::string task, std::vector<int> tokens)
    {
        if (task_to_id[task] == 0)
        {
            task_to_id[task] = ++task_num;
            task_to_time[task] = ++n_time;
            int task_ID = task_to_id[task];
            task_start[task_ID] = 1;
            std::vector<TrieNode*> NUL(16, nullptr);
            if (task_ptr.empty()) task_ptr.push_back(NUL);
            task_ptr.push_back(NUL);
//            task_num++;
//            n_time++;
        }

        int task_ID = task_to_id[task];
        int now = 0;
        std::deque<int> T;
        while (T.size() < 16 && now < tokens.size())
        {
            T.push_back(tokens[now]);
            if (task_ptr[task_ID][16 - T.size()] != nullptr && T.size() != 16)
            {
                insert(T, task_ptr[task_ID][16 - T.size()], task_to_time[task], task_ID);
                task_ptr[task_ID][16 - T.size()] = nullptr;
            }
            now ++;
        }
        while (T.size() == 16  && now < tokens.size()) {
            insert(T, root, task_to_time[task], task_ID);
            T.pop_front();
            now ++;
            if (now < tokens.size())
                T.push_back(tokens[now]);
        }
        if (T.size() == 16) {
            insert(T, root, task_to_time[task], task_ID);
            T.pop_front();
        }
        for (int i = 15; i > 0; i--) {
            if (task_ptr[task_ID][i] != nullptr)
                task_ptr[task_ID][i + T.size()] = insert(T, task_ptr[task_ID][i], task_to_time[task], task_ID);
        }
        while (!T.empty())
        {
            task_ptr[task_ID][T.size()] = insert(T, root, task_to_time[task], task_to_time[task]);
            T.pop_front();
        }

        del();
    }

    void seq_end(std::string task) {
        task_start[task_to_id[task]] = 0;
    }
};

Trie trie;

std::pair<std::vector<std::vector<int>>, std::vector<std::vector<float>>> get_Trie_tokens(std::vector<std::vector<int>> input, int top) {
    std::pair<std::vector<std::vector<int>>, std::vector<std::vector<float>>> results;
    for (auto tokens : input) {
        int flag = 0;
        for (int i = 0; i < tokens.size(); i++)
        {
            std::vector<int> T;
            for (auto j = tokens.begin() + i; j != tokens.end(); j++)  T.push_back(*j);
            std::pair<std::vector<int>, std::vector<float>> result = trie.find(T, top);
            if (!result.first.empty())
            {
                flag = 1;
                results.first.push_back(result.first);
                results.second.push_back(result.second);
                break;
            }
        }
        if (flag == 0)
        {
            std::vector<int> N;
            std::vector<float> NU;
            results.first.push_back(N);
            results.second.push_back(NU);
        }
    }
    return results;
}

void insert(std::string task, std::vector<int>tokens, int flag = 0) {
    trie.INSERT(task, tokens);
}

void seq_end(std::string task) {
    trie.seq_end(task);
}

PYBIND11_MODULE(Trie_token, m) {
    m.def("seq_end", &seq_end, "input txt path, Load data and build SA");
    m.def("insert", &insert, "input txt path, Load data and build SA");
    m.def("get_Trie_tokens", &get_Trie_tokens, "input&output vector<int>, Query suffix tokens");
}

int main() {
    insert("1", {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});
    insert("1", {17, 18, 19, 20, 21, 22, 23, 24});
    seq_end("1");
    insert("2", {1, 2, 4, 5});
    insert("2", {2, 3, 4, 5});
    std::pair<std::vector<std::vector<int>>, std::vector<std::vector<float>>>ans = get_Trie_tokens({{1, 2}}, 2);
    for (auto i : ans.first)
        for (auto out : i)
            std::cout << out << ' ';
    std::cout << std::endl;
    for (auto i : ans.second)
        for (auto out : i)
            std::cout << out << ' ';
    return 0;
}