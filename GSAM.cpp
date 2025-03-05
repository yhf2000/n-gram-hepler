#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <queue>
#include "nlohmann/json.hpp"
#include "absl/container/flat_hash_map.h"
#include <chrono>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <memory_resource>
using json = nlohmann::json;
namespace py = pybind11;

struct TreeNode {
    int val;
    std::vector<TreeNode*> children;
    TreeNode(int x) : val(x), children() {}
};

struct Node{
    int id, cnt;
};

class NaryTree {
private:
    void destroyTree(TreeNode* node) {
        if (node) {
            for (TreeNode* child : node->children) {
                destroyTree(child);
            }
            delete node;
        }
    }
public:
    TreeNode* root;
    NaryTree() : root(nullptr) {}
    ~NaryTree() {
        destroyTree(root);
    }
};

struct State {
    // absl::flat_hash_map<int, State*> trans;
    // absl::flat_hash_map<int, int> trans_count;
    std::unordered_map<int, State*> trans;
    std::unordered_map<int, int> trans_count;
    State* link;
    int len;
    State() : link(nullptr), len(0) {}
};

class GSAM {
// private:
//     std::pmr::monotonic_buffer_resource memory_pool;
//     std::pmr::polymorphic_allocator<State> alloc;

public:
    // std::pmr::vector<State*> all_states;
    std::vector<State*> all_states;
    State* root;
    State* last;

    GSAM()
        // : memory_pool(std::pmr::new_delete_resource()),
        //   alloc(&memory_pool),
        //   all_states(&memory_pool)
    {
        // root = alloc.allocate(1);
        // alloc.construct(root);
        root = new State();
        all_states.push_back(root);
        last = root;
    }

    ~GSAM() = default;

    void sa_extend(int c) {
        State* p = last;
        auto it = p->trans.find(c);
        if (it != p->trans.end()) {
            State* q = it->second;
            if (p->len + 1 == q->len) {
                last = q;
                p->trans_count[c]++;
                return;
            } else {
                // State* clone = alloc.allocate(1);
                // alloc.construct(clone);
                State* clone = new State();
                all_states.push_back(clone);
                clone->len = p->len + 1;
                clone->trans = q->trans;
                clone->trans_count = q->trans_count;
                clone->link = q->link;


                State* current_p = p;
                while (current_p != nullptr) {
                    auto it_trans = current_p->trans.find(c);
                    if (it_trans != current_p->trans.end() && it_trans->second == q) {
                        it_trans->second = clone;
                        current_p->trans_count[c]++;
                        current_p = current_p->link;
                    } else {
                        break;
                    }
                }

                clone->link = q->link;
                q->link = clone;
                last = clone;
                return;
            }
        } else {
            // State* curr = alloc.allocate(1);
            // alloc.construct(curr);
            State* curr = new State();
            all_states.push_back(curr);
            curr->len = p->len + 1;
            while (p != nullptr && p->trans.find(c) == p->trans.end()) {
                p->trans[c] = curr;
                p->trans_count[c]++;
                p = p->link;
            }

            if (p == nullptr) {
                curr->link = root;
            } else {
                State* q = p->trans[c];
                if (p->len + 1 == q->len) {
                    curr->link = q;
                } else {
                    // State* clone = alloc.allocate(1);
                    // alloc.construct(clone);
                    State* clone = new State();
                    all_states.push_back(clone);
                    clone->len = p->len + 1;
                    clone->trans = q->trans;
                    clone->trans_count = q->trans_count;
                    clone->link = q->link;

                    State* current_p = p;
                    while (current_p != nullptr) {
                        auto it_trans = current_p->trans.find(c);
                        if (it_trans != current_p->trans.end() && it_trans->second == q) {
                            it_trans->second = clone;
                            current_p->trans_count[c]++;
                            current_p = current_p->link;
                        } else {
                            break;
                        }
                    }

                    clone->link = q->link;
                    q->link = clone;
                    curr->link = clone;
                }
            }
            last = curr;
        }
    }

    void build(const std::vector<std::vector<int>>& tokens) {
        for (const auto & token : tokens) {
            last = root;
            for (int idx : token) {
                sa_extend(idx);
            }
        }
    }

    [[nodiscard]] State* get_root() {
        return root;
    }

    [[nodiscard]] State* find_state(const std::vector<int>& subtoken) const {
        State* current = root;
        for (int token : subtoken) {
            auto it = current->trans.find(token);
            if (it == current->trans.end()) {
                return nullptr;
            }
            current = it->second;
        }
        return current;
    }

    void serialize(const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) throw std::runtime_error("Cannot open file");

        // 建立指针到索引的映射
        std::unordered_map<State*, size_t> state_to_index;
        for (size_t i = 0; i < all_states.size(); ++i) {
            state_to_index[all_states[i]] = i;
        }

        // 写入状态总数
        const uint64_t state_count = all_states.size();
        file.write(reinterpret_cast<const char*>(&state_count), sizeof(state_count));

        // 写入每个状态
        for (State* s : all_states) {
            file.write(reinterpret_cast<const char*>(&s->len), sizeof(s->len));

            const int link_idx = (s->link) ? state_to_index[s->link] : -1;
            file.write(reinterpret_cast<const char*>(&link_idx), sizeof(link_idx));

            const uint64_t trans_size = s->trans.size();
            file.write(reinterpret_cast<const char*>(&trans_size), sizeof(trans_size));
            for (const auto& [key, val] : s->trans) {
                const int val_idx = state_to_index[val];
                file.write(reinterpret_cast<const char*>(&key), sizeof(key));
                file.write(reinterpret_cast<const char*>(&val_idx), sizeof(val_idx));
            }

            const uint64_t trans_count_size = s->trans_count.size();
            file.write(reinterpret_cast<const char*>(&trans_count_size), sizeof(trans_count_size));
            for (const auto& [key, count] : s->trans_count) {
                file.write(reinterpret_cast<const char*>(&key), sizeof(key));
                file.write(reinterpret_cast<const char*>(&count), sizeof(count));
            }

        }



        // 写入root和last的索引
        const int root_idx = state_to_index[root];
        const int last_idx = state_to_index[last];
        file.write(reinterpret_cast<const char*>(&root_idx), sizeof(root_idx));
        file.write(reinterpret_cast<const char*>(&last_idx), sizeof(last_idx));
    }

    void deserialize(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) throw std::runtime_error("Cannot open file");

        // 读取状态总数
        uint64_t state_count;
        file.read(reinterpret_cast<char*>(&state_count), sizeof(state_count));

        // 清理旧状态
        // memory_pool.release();  // 假设内存池支持release
        for (auto* state : all_states) delete state;
        all_states.clear();

        // 预分配所有状态
        all_states.reserve(state_count);
        for (uint64_t i = 0; i < state_count; ++i) {
            // State* s = alloc.allocate(1);  // 使用内存池分配
            // new (s) State();               // 调用构造函数
            all_states.push_back(new State());
        }

        // 读取每个状态
        for (uint64_t i = 0; i < state_count; ++i) {
            State* s = all_states[i];

            // 读取len
            file.read(reinterpret_cast<char*>(&s->len), sizeof(s->len));

            // 读取link索引并转换
            int link_idx;
            file.read(reinterpret_cast<char*>(&link_idx), sizeof(link_idx));
            s->link = (link_idx == -1) ? nullptr : all_states[link_idx];

            // 读取trans映射
            uint64_t trans_size;
            file.read(reinterpret_cast<char*>(&trans_size), sizeof(trans_size));
            for (uint64_t j = 0; j < trans_size; ++j) {
                int key, val_idx;
                file.read(reinterpret_cast<char*>(&key), sizeof(key));
                file.read(reinterpret_cast<char*>(&val_idx), sizeof(val_idx));
                s->trans[key] = all_states[val_idx];
            }

            uint64_t trans_count_size;
            file.read(reinterpret_cast<char*>(&trans_count_size), sizeof(trans_count_size));
            for (uint64_t j = 0; j < trans_count_size; ++j) {
                int key, count;
                file.read(reinterpret_cast<char*>(&key), sizeof(key));
                file.read(reinterpret_cast<char*>(&count), sizeof(count));
                s->trans_count[key] = count;
            }
        }

        // 读取root和last
        int root_idx, last_idx;
        file.read(reinterpret_cast<char*>(&root_idx), sizeof(root_idx));
        file.read(reinterpret_cast<char*>(&last_idx), sizeof(last_idx));
        root = all_states[root_idx];
        last = all_states[last_idx];
    }

    void find(TreeNode *curr, const int k2, int top, std::vector<int>& long_token)
    {
        State* current = root;
        for (int token : long_token) {
            auto it = current->trans.find(token);
            if (it == current->trans.end()) {
                return ;
            }
            current = it->second;
        }
        std::queue<std::pair<State*, std::pair<TreeNode *, int>>>q;
        q.push({current, {curr, k2}});
        while (!q.empty())
        {
            State* now_current = q.front().first;
            TreeNode *now_tree_curr = q.front().second.first;
            int res_len = q.front().second.second;
            q.pop();
            std::vector<Node> son;
            for (auto child : now_current->trans)
            {
                int token = child.first;
                son.push_back({token, now_current->trans_count[token]});
            }
            std::sort(son.begin(), son.end(), [](Node a, Node b) {return a.cnt < b.cnt;});
            for (int i = 0; i < std::min(int(son.size()), top); i++)
            {
                now_tree_curr->children.push_back(new TreeNode(son[i].id));
                if (res_len != 0)
                    q.push({now_current->trans[son[i].id], {now_tree_curr->children.back(), res_len - 1}});
            }
        }
    }
};

GSAM gsam;

void add_query_substring_suffixes(State* state, std::vector<int>& results) {
    for (const auto& pair : state->trans) {
        int token = pair.first;
        results.push_back(token);
        add_query_substring_suffixes(pair.second, results);
    }
}

std::vector<std::vector<int>> data_load(const std::string& data_path) {
    std::vector<std::vector<int>> dataset;
    std::ifstream infile(data_path);

    if (!infile.is_open()) {
        std::cerr << "Error: Unable to open file " << data_path << std::endl;
        exit(1);
    }

    std::string line;
    while (std::getline(infile, line)) {
        try {
            json json_obj = json::parse(line);

            if (json_obj.contains("tokens") && json_obj["tokens"].is_array()) {
                std::vector<int> tokens;
                for (const auto& token : json_obj["tokens"]) {
                    tokens.push_back(token.get<int>());
                }
                dataset.push_back(tokens);
            }
        } catch (const json::exception& e) {
            std::cerr << "JSON parsing error: " << e.what() << std::endl;
            exit(1);
        }
    }

    infile.close();
    return dataset;
}

void set_GSAM_data(const std::string& data_path)
{
    std::vector<std::vector<int>> test_tokens = data_load(data_path);
    gsam.build(test_tokens);
}

std::vector<int>get_GSAM_tokens(const std::vector<int>& input_token) {
    State* state = gsam.find_state(input_token);
    if (!state) {
        return {};
    }
    std::vector<int> results;
    add_query_substring_suffixes(state, results);
    return results;
}

void save_data(const std::string& data_path) {
    gsam.serialize(data_path);
}

void load_data(const std::string& data_path) {
    gsam.deserialize(data_path);
}

void check(TreeNode *curr, const int k1, const int k2, const int top, std::vector<int>& long_token)
{
    long_token.push_back(curr->val);
    if (curr->children.size() == 0)
    {
        if (long_token.size() >= k1)
        {
            std::vector<int> tokens;
            for (int i = long_token.size() - k1; i < long_token.size(); i++)
                tokens.push_back(long_token[i]);
            gsam.find(curr, k2, top, tokens);
        }
    }
    else
        for (auto to : curr->children)
            check(to, k1, k2, top, long_token);
    long_token.pop_back();
}


std::pair<std::vector<int>, std::vector<int>>  tree_get_GSAM_tokens(const std::vector<int>& pre_token,
                                                             const std::vector<int>& input_tree,
                                                             const std::vector<int>& input_token,
                                                             const int& k1, const int& k2, const int& top)
{
    NaryTree tree;
    TreeNode *curr = tree.root;
    for (auto token : pre_token)
    {
        if (tree.root == nullptr) tree.root = new TreeNode(token), curr = tree.root;
        else
        {
            curr->children.push_back(new TreeNode(token));
            curr = curr->children.back();
        }
    }

    TreeNode *branch_curr = curr;

    std::queue<TreeNode *> q;
    q.push(curr);
    int token_now = 0, tree_now = 0;
    while (!q.empty() && token_now < input_token.size() && tree_now < input_token.size())
    {
        TreeNode *cur = q.front(); q.pop();
        for (int i = token_now; i < token_now + input_tree[tree_now]; ++i)
        {
            cur->children.push_back(new TreeNode(input_token[i]));
            q.push(cur->children.back());
        }
        token_now += input_tree[tree_now];
        tree_now ++;
    }
    while (!q.empty()) q.pop();
    std::vector<int> token;
    std::vector<int> tree_shape;
    std::vector<int> long_token;
    check(tree.root, k1, k2, top, long_token);

    q.push(branch_curr);
    while (!q.empty())
    {
        TreeNode *cur = q.front(); q.pop();
        tree_shape.push_back(cur->children.size());
        token.push_back(cur->val);
        for (auto child : cur->children)
            q.push(child);
    }
    int input_tree_len = input_tree.size();
    int input_token_len = input_token.size();
    tree_shape.erase(tree_shape.begin(), tree_shape.begin() + input_tree_len);
    token.erase(token.begin(), token.begin() + input_token_len);


    return {tree_shape, token};
}

PYBIND11_MODULE(GSAM_token, m) {
    m.def("set_GSAM_data", &set_GSAM_data, "input jsonl, Load data and build csrc");
    m.def("get_GSAM_tokens", &get_GSAM_tokens, "input&output vector<int>, Query suffix tokens");
    m.def("save_data", &save_data, "input path, save data");
    m.def("load_data", &load_data, "input path, load data");
    m.def("tree_get_GSAM_tokens", &tree_get_GSAM_tokens, "input pre_token, tree, token, k1, k2, top; return tree, token");
}

int main(){
    auto start = std::chrono::high_resolution_clock::now();
    set_GSAM_data("/mnt/data1/lth/GSAM_tokens/datastore_token_test.jsonl");
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << "Elapsed time: " << elapsed_seconds.count() << "seconds" << std::endl;
}





// std::vector<int> query_substring_suffixes(csrc& gsam, const std::vector<int>& subtoken) {
//     State* state = gsam.find_state(subtoken);
//     if (!state) {
//         return {};
//     }
//     std::vector<int> results;
//     add_query_substring_suffixes(state, results);
//     return results;
// }
//统计子串出现次数，失败的复杂度


