#include <queue>
#include <vector>
#include <cstring>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <ostream>
#include <sstream>
#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
const int ENDPOS = 2147483647;
namespace py = pybind11;


struct SuffixAutomatonSet {
    struct SuffixAutomaton {
        struct node{
            int id = 0;
            float cnt = 0;
            bool operator<(const node& other) const {
                return (cnt == other.cnt ? id < other.id : cnt > other.cnt);
            }
        };

        struct LinkCutTree {
            struct Node {
                Node *ch[2], *fa, *pathFa;
                int val, tag;
                bool rev;

                int relation() {
                    return this == fa->ch[1];
                }

                void pushDown() {
                    if (rev) {
                        rev ^= 1;
                        std::swap(ch[0], ch[1]);
                        if (ch[0]) ch[0]->rev ^= 1;
                        if (ch[1]) ch[1]->rev ^= 1;
                    }

                    if (tag) {
                        if (ch[0]) ch[0]->add(tag);
                        if (ch[1]) ch[1]->add(tag);
                        tag = 0;
                    }
                }

                void rotate() {
                    pushDown();

                    int r = relation();
                    Node *o = fa;

                    std::swap(pathFa, o->pathFa);

                    fa = o->fa;
                    if (o->fa) o->fa->ch[o->relation()] = this;

                    o->ch[r] = ch[r ^ 1];
                    if (ch[r ^ 1]) ch[r ^ 1]->fa = o;

                    ch[r ^ 1] = o;
                    o->fa = this;
                }

                void splay() {
                    while (fa) {
                        if (fa->fa) fa->fa->pushDown();
                        fa->pushDown();

                        if (!fa->fa) rotate();
                        else if (fa->relation() == relation()) fa->rotate(), rotate();
                        else rotate(), rotate();
                    }
                }

                void expose() {
                    splay();
                    pushDown();
                    if (ch[1]) {
                        std::swap(ch[1]->fa, ch[1]->pathFa);
                        ch[1] = nullptr;
                    }
                }

                bool splice() {
                    splay();
                    if (!pathFa) return false;

                    pathFa->expose();
                    pathFa->ch[1] = this;
                    std::swap(pathFa, fa);

                    return true;
                }

                void access() {
                    expose();
                    while (splice());
                }

                void evert() {
                    access();
                    splay();
                    rev ^= 1;
                }

                void add(int delta) {
                    val += delta;
                    tag += delta;
                }
            } ;

            std::unordered_map<int, Node> N;

            void link(int fa, int ch) {
                Node *f = &N[fa], *c = &N[ch];

                f->evert();
                c->splay();
                c->pathFa = f;

                N[0].evert();

                f->access();
                f->splay();
                f->val += c->val;
                if (f->ch[0]) f->ch[0]->add(c->val);
            }

            void cut(int fa, int ch) {
                Node *f = &N[fa], *c = &N[ch];

                f->evert();
                c->access();
                f->splay();
                f->pushDown();

                f->ch[1] = nullptr;
                c->fa = nullptr;

                N[0].evert();

                f->access();
                f->splay();
                f->val -= c->val;
                if (f->ch[0]) f->ch[0]->add(-c->val);
            }

            void update(int u, int val) {
                N[u].val = val;
            }

            int query(int u) {
                Node *a = &N[u];
                a->splay();
                return a->val;
            }
        } lct;

        struct Node {
            std::unordered_map<int, Node*>ch;
            Node *next;
            int max;

            Node(int max = 0) : ch(), next(nullptr), max(max) {}
        } *start, *last;
        std::unordered_map<Node*, int> Node_id;

        int Node_cnt;

        SuffixAutomaton() {
            Node_cnt = 0;
            start = last = new Node;
            Node_id[start] = (Node_cnt++);
        }

        int id(Node *v) {
            return Node_id[v];
        }
        Node *extend(int c) {
            Node *u = new Node(last->max + 1), *v = last;
            Node_id[u] = (Node_cnt++);
            lct.update(id(u), 1);

            do  {
                v->ch[c] = u;
                v = v->next;
            } while(v && v->ch[c] == nullptr);

            if (!v) {
                u->next = start;

                lct.link(id(start), id(u));
            } else if (v->ch[c]->max == v->max + 1) {
                u->next = v->ch[c];

                lct.link(id(v->ch[c]), id(u));
            } else {
                Node *n = new Node(v->max + 1), *o = v->ch[c];
                Node_id[n] = (Node_cnt++);
                for (auto i : o->ch)
                    n->ch[i.first] = i.second;

                lct.cut(id(o->next), id(o));
                lct.link(id(o->next), id(n));
                lct.link(id(n), id(o));
                lct.link(id(n), id(u));

                n->next = o->next;
                o->next = u->next = n;
                for (; v && v->ch[c] == o; v = v->next) v->ch[c] = n;
            }

            last = u;
            return u;
        }

        std::pair<std::vector<int>, std::vector<float>>find(const std::vector<int>& Tokens, int top) {
            std::pair<std::vector<int>, std::vector<float>> result;
            std::vector<int> T = Tokens;
            Node *cur = start;
            for (auto token : T) {
                if (cur->ch.find(token) == cur->ch.end()) return result;
                cur = cur->ch[token];
            }
            std::vector<node> members;
            for (auto t : cur->ch) {
                if (t.first == ENDPOS) continue;
                float cnt = lct.query(id(t.second));
                members.push_back({t.first, cnt});
            }
            sort(members.begin(), members.end());
            float num = 0;
            for (int i = 0; i < top && i < members.size(); i++)
                num += members[i].cnt;
            for (int i = 0; i < top && i < members.size(); i++)
            {
                result.first.push_back(members[i].id);
                result.second.push_back(members[i].cnt / num);
            }
            return result;
        }
    };
    SuffixAutomaton HistoryGSAM;
    std::unordered_map<std::string, int> task_id;
    std::vector<std::vector<int>> tokens;
    std::vector<SuffixAutomaton> GSAM;
    int task_cnt;
    SuffixAutomatonSet() {
        task_cnt = 0;
    }
}GSAMS;

void insert(const std::string& task, const std::vector<int>& tokens) {
    if (GSAMS.task_id[task] == 0)
    {
        GSAMS.task_id[task] = GSAMS.task_cnt;
        SuffixAutomatonSet::SuffixAutomaton N;
        std::vector<int> NU;
        GSAMS.GSAM.push_back(N);
        GSAMS.tokens.push_back(NU);
        GSAMS.task_cnt++;
    }
    int taskID = GSAMS.task_id[task];
    for (auto token : tokens)
    {
        GSAMS.GSAM[taskID].extend(token);
        GSAMS.tokens[taskID].push_back(token);
    }
}

void seq_end(std::string& task) {
    int taskID = GSAMS.task_id[task];
    for (auto token : GSAMS.tokens[taskID])
        GSAMS.HistoryGSAM.extend(token);
    GSAMS.HistoryGSAM.extend(ENDPOS);
}

std::pair<std::pair<std::vector<int>, std::vector<float>>, std::pair<std::vector<int>, std::vector<float>>> get_GSAM_tokens(const std::string& task, const std::vector<int>& tokens, const int top) {
    int taskID = GSAMS.task_id[task];
    int flag = false;
    if (GSAMS.tokens.size() >= taskID - 1 && GSAMS.tokens[taskID].size() > 0)
        flag = true;
    for (int i = 0; i < tokens.size(); i++)
    {
        std::vector<int> T;
        for (auto j = tokens.begin() + i; j != tokens.end(); ++j)  T.push_back(*j);
        std::pair<std::vector<int>, std::vector<float>> Result;
        std::pair<std::vector<int>, std::vector<float>> HistoryResult;
        if (flag)  Result = GSAMS.GSAM[taskID].find(T, top);
        HistoryResult = GSAMS.HistoryGSAM.find(T, top);

        if (!Result.first.empty() || !HistoryResult.first.empty())
            return {Result, HistoryResult};
    }
    std::vector<int> N;
    std::vector<float> NU;
    return {{N, NU}, {N, NU}};
}


void serialize_lct(const SuffixAutomatonSet::SuffixAutomaton::LinkCutTree& lct, std::ofstream& ofs) {
    // 写入节点数量
    size_t node_count = lct.N.size();
    ofs.write(reinterpret_cast<const char*>(&node_count), sizeof(node_count));

    // 遍历所有节点
    for (const auto& [key, node] : lct.N) {
        // 写入键
        ofs.write(reinterpret_cast<const char*>(&key), sizeof(key));

        // 写入数值属性（val, tag, rev）
        ofs.write(reinterpret_cast<const char*>(&node.val), sizeof(node.val));
        ofs.write(reinterpret_cast<const char*>(&node.tag), sizeof(node.tag));
        ofs.write(reinterpret_cast<const char*>(&node.rev), sizeof(node.rev));

        // 指针转换为键（假设用-1表示空指针）
        int ch0_key = (node.ch[0] ? reinterpret_cast<intptr_t>(node.ch[0]) : -1);
        int ch1_key = (node.ch[1] ? reinterpret_cast<intptr_t>(node.ch[1]) : -1);
        int fa_key = (node.fa ? reinterpret_cast<intptr_t>(node.fa) : -1);
        int pathFa_key = (node.pathFa ? reinterpret_cast<intptr_t>(node.pathFa) : -1);

        ofs.write(reinterpret_cast<const char*>(&ch0_key), sizeof(ch0_key));
        ofs.write(reinterpret_cast<const char*>(&ch1_key), sizeof(ch1_key));
        ofs.write(reinterpret_cast<const char*>(&fa_key), sizeof(fa_key));
        ofs.write(reinterpret_cast<const char*>(&pathFa_key), sizeof(pathFa_key));
    }
}

void serialize_nodes(const SuffixAutomatonSet::SuffixAutomaton& sa, std::ofstream& ofs) {
    // 写入节点总数
    ofs.write(reinterpret_cast<const char*>(&sa.Node_cnt), sizeof(sa.Node_cnt));

    // 遍历所有节点（假设有遍历方法）
    for (const auto& [node_ptr, id] : sa.Node_id) {
        // 写入节点ID
        ofs.write(reinterpret_cast<const char*>(&id), sizeof(id));

        // 写入max属性
        ofs.write(reinterpret_cast<const char*>(&node_ptr->max), sizeof(node_ptr->max));

        // 序列化子节点映射（将指针转换为ID）
        size_t ch_size = node_ptr->ch.size();
        ofs.write(reinterpret_cast<const char*>(&ch_size), sizeof(ch_size));
        for (const auto& [edge, child] : node_ptr->ch) {
            // 写入边值和子节点ID
            ofs.write(reinterpret_cast<const char*>(&edge), sizeof(edge));
            int child_id = sa.Node_id.at(child);
            ofs.write(reinterpret_cast<const char*>(&child_id), sizeof(child_id));
        }

        // 序列化next指针（转换为ID）
        int next_id = (node_ptr->next ? sa.Node_id.at(node_ptr->next) : -1);
        ofs.write(reinterpret_cast<const char*>(&next_id), sizeof(next_id));
    }
}

void serialize_sam(const SuffixAutomatonSet::SuffixAutomaton& sam, std::ofstream& ofs) {
    // 序列化LinkCutTree
    serialize_lct(sam.lct, ofs);

    // 序列化节点树
    serialize_nodes(sam, ofs);

    // 保存start和last的ID（用-1表示空指针）
    int start_id = (sam.start ? sam.Node_id.at(sam.start) : -1);
    int last_id = (sam.last ? sam.Node_id.at(sam.last) : -1);
    ofs.write(reinterpret_cast<const char*>(&start_id), sizeof(start_id));
    ofs.write(reinterpret_cast<const char*>(&last_id), sizeof(last_id));
}

void deserialize_lct(SuffixAutomatonSet::SuffixAutomaton::LinkCutTree& lct, std::ifstream& ifs) {
    // 读取节点数量
    size_t node_count;
    ifs.read(reinterpret_cast<char*>(&node_count), sizeof(node_count));

    // 临时存储指针键到实际指针的映射
    std::unordered_map<int, SuffixAutomatonSet::SuffixAutomaton::LinkCutTree::Node*> ptr_map;

    for (size_t i = 0; i < node_count; ++i) {
        int key;
        ifs.read(reinterpret_cast<char*>(&key), sizeof(key));

        // 创建新节点并插入到unordered_map
        auto& node = lct.N[key];
        ptr_map[key] = &node;

        // 读取数值属性
        ifs.read(reinterpret_cast<char*>(&node.val), sizeof(node.val));
        ifs.read(reinterpret_cast<char*>(&node.tag), sizeof(node.tag));
        ifs.read(reinterpret_cast<char*>(&node.rev), sizeof(node.rev));

        // 读取指针键（稍后链接）
        int ch0_key, ch1_key, fa_key, pathFa_key;
        ifs.read(reinterpret_cast<char*>(&ch0_key), sizeof(ch0_key));
        ifs.read(reinterpret_cast<char*>(&ch1_key), sizeof(ch1_key));
        ifs.read(reinterpret_cast<char*>(&fa_key), sizeof(fa_key));
        ifs.read(reinterpret_cast<char*>(&pathFa_key), sizeof(pathFa_key));

        // 临时保存键，稍后替换为实际指针
        node.ch[0] = (ch0_key != -1) ? ptr_map[ch0_key] : nullptr;
        node.ch[1] = (ch1_key != -1) ? ptr_map[ch1_key] : nullptr;
        node.fa = (fa_key != -1) ? ptr_map[fa_key] : nullptr;
        node.pathFa = (pathFa_key != -1) ? ptr_map[pathFa_key] : nullptr;
    }
}

// 修改 deserialize_nodes 函数，增加 id_map 参数
void deserialize_nodes(
    SuffixAutomatonSet::SuffixAutomaton& sam,
    std::ifstream& ifs,
    std::unordered_map<int, SuffixAutomatonSet::SuffixAutomaton::Node*>& id_map
) {
    // 读取节点总数
    ifs.read(reinterpret_cast<char*>(&sam.Node_cnt), sizeof(sam.Node_cnt));

    id_map.clear();

    // 临时存储子节点和 next 的 ID
    std::unordered_map<int, std::vector<std::pair<int, int>>> temp_children; // id -> [(edge, child_id)]
    std::unordered_map<int, int> temp_next; // id -> next_id

    // 第一阶段：创建所有节点并记录临时 ID
    for (int i = 0; i < sam.Node_cnt; ++i) {
        int id;
        ifs.read(reinterpret_cast<char*>(&id), sizeof(id));

        // 创建新节点
        auto* node = new SuffixAutomatonSet::SuffixAutomaton::Node;
        sam.Node_id[node] = id;
        id_map[id] = node;

        // 读取 max 属性
        ifs.read(reinterpret_cast<char*>(&node->max), sizeof(node->max));

        // 读取子节点映射到临时结构
        size_t ch_size;
        ifs.read(reinterpret_cast<char*>(&ch_size), sizeof(ch_size));
        std::vector<std::pair<int, int>> children;
        for (size_t j = 0; j < ch_size; ++j) {
            int edge, child_id;
            ifs.read(reinterpret_cast<char*>(&edge), sizeof(edge));
            ifs.read(reinterpret_cast<char*>(&child_id), sizeof(child_id));
            children.emplace_back(edge, child_id);
        }
        temp_children[id] = std::move(children);

        // 读取 next 的 ID 到临时结构
        int next_id;
        ifs.read(reinterpret_cast<char*>(&next_id), sizeof(next_id));
        temp_next[id] = next_id;
    }

    // 第二阶段：将临时 ID 转换为指针
    for (auto& [id, node] : id_map) {
        // 处理子节点
        auto& children = temp_children[id];
        for (auto [edge, child_id] : children) {
            node->ch[edge] = id_map.at(child_id); // 通过 ID 获取指针
        }

        // 处理 next 指针
        int next_id = temp_next[id];
        node->next = (next_id != -1) ? id_map.at(next_id) : nullptr;
    }
}

// 修改 deserialize_sam 函数
void deserialize_sam(SuffixAutomatonSet::SuffixAutomaton& sam, std::ifstream& ifs) {
    // 反序列化 LinkCutTree
    deserialize_lct(sam.lct, ifs);

    // 反序列化节点树（传入 id_map）
    std::unordered_map<int, SuffixAutomatonSet::SuffixAutomaton::Node*> id_map;
    deserialize_nodes(sam, ifs, id_map);  // 填充 id_map

    // 恢复 start 和 last 指针
    int start_id, last_id;
    ifs.read(reinterpret_cast<char*>(&start_id), sizeof(start_id));
    ifs.read(reinterpret_cast<char*>(&last_id), sizeof(last_id));
    sam.start = (start_id != -1) ? id_map.at(start_id) : nullptr;
    sam.last = (last_id != -1) ? id_map.at(last_id) : nullptr;
}

void save_history_gsam(const std::string& filename) {
    std::ofstream ofs(filename, std::ios::binary);
    serialize_sam(GSAMS.HistoryGSAM, ofs);
}

// 从文件加载HistoryGSAM
void load_history_gsam(const std::string& filename) {
    std::ifstream ifs(filename, std::ios::binary);
    deserialize_sam(GSAMS.HistoryGSAM, ifs);
}

std::string trim_line(const std::string& s) {
    std::string result;
    result.reserve(s.size()); // 预分配空间以提高效率

    // 遍历每个字符，过滤掉 '[', ']', ','
    for (char c : s) {
        if (c != '[' && c != ']' && c != ',') {
            result.push_back(c);
        } else {
            result.push_back(' '); // 将不需要的字符替换为空格
        }
    }

    // 去除首尾空格
    size_t start = result.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = result.find_last_not_of(" \t\n\r");
    std::string trimmed = result.substr(start, end - start + 1);

    // 合并连续的空格为单个空格
    std::string final_str;
    bool prev_space = false;
    for (char c : trimmed) {
        if (std::isspace(c)) {
            if (!prev_space) {
                final_str.push_back(' ');
                prev_space = true;
            }
        } else {
            final_str.push_back(c);
            prev_space = false;
        }
    }

    return final_str;
}

std::vector<std::vector<int>> read_multiline_integer_file(const std::string& filename) {
    std::vector<std::vector<int>> result;  // 二维数组
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return result;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::vector<int> row;  // 当前行的数值

        // 清理行内容
        std::string trimmed_line = trim_line(line);
        if (trimmed_line.empty()) {
            continue;  // 跳过空行
        }

        // 解析当前行
        std::stringstream ss(trimmed_line);
        int num;
        while (ss >> num) {
            row.push_back(num);
        }

        // 检查是否有未解析内容
        if (!ss.eof()) {
            ss.clear();
            std::string remaining;
            ss >> remaining;
            std::cerr << "警告: 行内容包含无效字符 [" << remaining << "]: " << line << std::endl;
        }
        row.push_back(ENDPOS);
        result.push_back(row);  // 添加当前行到结果
    }

    file.close();
    return result;
}

void build_history_gsam(const std::string& load_filename, const std::string& save_filename) {
    std::vector<std::vector<int>> tokens = read_multiline_integer_file(load_filename);
    for (const auto& token : tokens)
        for (auto c : token)
            GSAMS.HistoryGSAM.extend(c);
    save_history_gsam(save_filename);
}

PYBIND11_MODULE(GSAM_token, m) {
    m.def("build_history_gsam", &build_history_gsam, "input load_filename and save_filename");
    m.def("load_history_gsam", &load_history_gsam, "input save_filename");
    m.def("insert", &insert, "input txt path, Load data and build SA");
    m.def("seq_end", &seq_end, "input task");
    m.def("get_SAM_tokens", &get_GSAM_tokens, "input&output vector<int>, Query suffix tokens");
}

int main() {
    // build_history_gsam("/data/share/SA_data/datastore_chat_small.txt", "/data/share/SA_data/datastore_chat_small.bin");
    load_history_gsam("/data/share/SA_data/datastore_chat_small.bin");
    std::pair<std::pair<std::vector<int>, std::vector<float>>, std::pair<std::vector<int>, std::vector<float>>> result = get_GSAM_tokens("1", {128000}, 10);
    std::pair<std::vector<int>, std::vector<float>> Result = result.first;
    std::pair<std::vector<int>, std::vector<float>> HistoryResult = result.second;
    for (auto i : Result.first) std::cout << i << ' '; puts("");
    for (auto i : Result.second) std::cout << i << ' '; puts("");
    for (auto i : HistoryResult.first) std::cout << i << ' '; puts("");
    for (auto i : HistoryResult.second) std::cout << i << ' '; puts("");
    return 0;
}