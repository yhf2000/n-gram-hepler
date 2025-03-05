#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <vector>
#include <queue>
#include <set>
#include <algorithm>

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


struct TrieNode {
    int vis_count = 0;
    int deepth = 0;
    std::unordered_map<int, TrieNode*> children;
};
std::unordered_map<TrieNode*, int> node_nearest_time;
struct CompareByNearestTime {
    bool operator()(TrieNode* a, TrieNode* b) const {
        return (a->deepth == b->deepth ? a->deepth > b->deepth : node_nearest_time[a] < node_nearest_time[b]);
    }
};


class Trie {
private:
    int branch_length;
    int max_node;
    int n_node;
    int n_time;
    TrieNode* root;
    std::set<TrieNode*, CompareByNearestTime> del_node;
    void clear(TrieNode* node) {
        if (!node) return;
        for (auto& pair : node->children) {
            clear(pair.second);
        }
        n_node--;
        delete node;
    }


public:
    Trie() : root(new TrieNode()), max_node(65535), n_node(0), branch_length(8), n_time(0) {}

    ~Trie() {
        clear(root);
    }

    void del()
    {
        while (n_node > max_node)
        {
            del_node.erase(del_node.begin());
            n_node ++;
        }
    }

    void insert(const std::vector<int>& tokens, float freq = 1.0) {
        ++n_time;
        TrieNode* current = root;
        for (int token : tokens) {
            if (!current->children.count(token)) {
                current->children[token] = new TrieNode();
                current->children[token]->deepth = current->deepth + 1;
                node_nearest_time[current->children[token]] = n_time;
                n_node++;
                del_node.insert(current->children[token]);
            }
            current->vis_count++;
            current = current->children[token];
        }
        if (n_node > max_node)
            del();
    }

    void set(int Branch_length, int Max_node)
    {
        branch_length = Branch_length;
        max_node = Max_node;
    }

    void find(TreeNode *curr, const int k2, int top, std::vector<int>& long_token)
    {
        TrieNode* current = root;
        for (int token : long_token) {
            auto it = current->children.find(token);
            if (it == current->children.end()) {
                return ;
            }
            current = it->second;
        }
        std::queue<std::pair<TrieNode*, std::pair<TreeNode *, int>>>q;
        q.push({current, {curr, k2}});
        while (!q.empty())
        {
            TrieNode* now_current = q.front().first;
            TreeNode *now_tree_curr = q.front().second.first;
            int res_len = q.front().second.second;
            q.pop();
            std::vector<Node> son;
            for (auto child : now_current->children)
            {
                int token = child.first;
                son.push_back({token, now_current->children[token]->vis_count});
            }
            std::sort(son.begin(), son.end(), [](Node a, Node b) {return a.cnt < b.cnt;});
            for (int i = 0; i < std::min(int(son.size()), top); i++)
            {
                now_tree_curr->children.push_back(new TreeNode(son[i].id));
                if (res_len != 0)
                {
                    q.push({now_current->children[son[i].id], {now_tree_curr->children.back(), res_len - 1}});

                }
            }
        }
    }
};

Trie trie;


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
            trie.find(curr, k2, top, tokens);
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

int main() {

    return 0;
}