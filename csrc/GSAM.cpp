#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <queue>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
const int ENDPOS = 2147483647;
namespace py = pybind11;

struct SuffixAutomatonSet {
    struct SuffixAutomaton {
        struct node{
            int id = 0, cnt = 0;
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

        std::pair<std::vector<int>, std::vector<float>>find(const std::vector<int>& tokens, int top) {
            std::pair<std::vector<int>, std::vector<float>> result;
            std::vector<int> T = tokens;
            Node *cur = start;
            for (auto token : T) {
                if (cur->ch.find(token) == cur->ch.end()) return result;
                cur = cur->ch[token];
            }
            std::vector<node> members;
            for (auto t : cur->ch) {
                if (t.first == ENDPOS) continue;
                int cnt = lct.query(id(t.second));
                members.push_back({t.first, cnt});
            }
            sort(members.begin(), members.end());
            int num = 0;
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
    for (int i = 0; i < tokens.size(); i++)
    {
        std::vector<int> T;
        for (auto j = tokens.begin() + i; j != tokens.end(); ++j)  T.push_back(*j);
        std::pair<std::vector<int>, std::vector<float>> Result = GSAMS.GSAM[taskID].find(T, top);
        std::pair<std::vector<int>, std::vector<float>> HistoryResult = GSAMS.HistoryGSAM.find(T, top);

        if (!Result.first.empty() || !HistoryResult.first.empty())
            return {Result, HistoryResult};
    }
    std::vector<int> N;
    std::vector<float> NU;
    return {{N, NU}, {N, NU}};
}


PYBIND11_MODULE(GSAM_token, m) {
    m.def("insert", &insert, "input txt path, Load data and build SA");
    m.def("seq_end", &seq_end, "input task");
    m.def("get_SAM_tokens", &get_GSAM_tokens, "input&output vector<int>, Query suffix tokens");
}



int main() {


    return 0;
}