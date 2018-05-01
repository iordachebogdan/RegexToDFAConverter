#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <queue>
using namespace std;

class DFA {
  public:
    DFA() = default;
    DFA(const DFA&) = default;
    DFA& operator=(const DFA&) = default;
    virtual ~DFA() = default;

    int size() const {return (int)nodes_.size();}
    void add_node(int node) {nodes_.insert(node);}

    void set_initial(int node) {add_node(node); initial_ = node;}
    void add_final(int node) {add_node(node); final_nodes_.insert(node);}

    void add_transition(int from, char letter, int to) {
        add_node(from);
        add_node(to);
        transitions_[make_pair(from, letter)] = to;
    }

    friend ostream& operator << (ostream& out, const DFA& dfa) {
        //output states
        out << dfa.size() << '\n';
        for (int node : dfa.nodes_)
            out << node << " ";
        out << '\n';
        //output letters
        unordered_set< char > letters;
        for (auto it : dfa.transitions_)
            letters.insert(it.first.second);
        out << letters.size() << '\n';
        for (char letter : letters)
            out << letter << " ";
        out << '\n';
        //output initial state
        out << dfa.initial_ << '\n';
        //output final states
        out << dfa.final_nodes_.size() << '\n';
        for (int node : dfa.final_nodes_)
            out << node << ' ';
        out << '\n';
        //output transitions
        out << dfa.transitions_.size() << '\n';
        for (auto it : dfa.transitions_)
            out << it.first.first << ' ' << it.first.second << ' ' << it.second << '\n';
        return out;
    }
  private:
    unordered_set<int> nodes_;
    unordered_set<int> final_nodes_;
    int initial_;
    map<pair<int, char>, int> transitions_;
};

class RegexToDFAConverter {
  private:
    struct SyntaxTreeNode;
  public:
    RegexToDFAConverter() = default;
    RegexToDFAConverter(const RegexToDFAConverter&) = default;
    RegexToDFAConverter& operator=(const RegexToDFAConverter&) = default;
    virtual ~RegexToDFAConverter() = default;

    DFA convert(const string& expr) {
        string simplified_expr = simplify(expr);
        int curr_pos = 0;
        leaves_count_ = 0, follow_pos_.clear(); what_letter_.clear();
        auto syntax_tree_root = expression_eval(simplified_expr, curr_pos);

        auto comp = [&](int a, int b) -> bool {
            return (what_letter_[a] == what_letter_[b] ? a < b : what_letter_[a] < what_letter_[b]);
        };

        queue< vector<int> > que;
        que.push(vector<int>());
        for (int it : syntax_tree_root->first_pos)
            que.front().push_back(it);
        sort(que.front().begin(), que.front().end(), comp);

        mark_.clear();
        int num_states = 0;
        DFA res;
        mark_[que.front()] = ++num_states;
        res.add_node(num_states);
        res.set_initial(num_states);
        delete syntax_tree_root;

        while (!que.empty()) {
            auto curr_state = que.front();
            que.pop();

            int i = 0;
            while (i < (int)curr_state.size()) {
                int j = i;
                vector< int > next;
                while (j < (int)curr_state.size() && what_letter_[curr_state[i]] == what_letter_[curr_state[j]]) {
                    for (int it : follow_pos_[curr_state[j]])
                        next.push_back(it);
                    j++;
                }
                if (what_letter_[curr_state[i]] == '#') {
                    i = j;
                    continue;
                }
                sort(next.begin(), next.end());
                next.erase(unique(next.begin(), next.end()), next.end());
                sort(next.begin(), next.end(), comp);

                if (!mark_[next]) {
                    mark_[next] = ++num_states;
                    res.add_node(num_states);
                    que.push(next);
                }

                res.add_transition(mark_[curr_state], what_letter_[curr_state[i]], mark_[next]);
                i = j;
            }
        }

        for (auto it : mark_)
            for (int state : it.first)
                if (what_letter_[state] == '#') {
                    res.add_final(it.second);
                    break;
                }

        return res;
    }
  private:
    struct SyntaxTreeNode {
        SyntaxTreeNode() : leaf_index(0), nullable(false) {}
        ~SyntaxTreeNode() { wipe_sons(); }
        void wipe_sons() {
            for (auto it : sons)
                delete it;
            sons.clear();
        }

        vector< SyntaxTreeNode* > sons;
        unordered_set< int > first_pos, last_pos;
        int leaf_index; //0 if not leaf or leaf with lambda
        bool nullable;
    };

    string simplify(const string& expr) {
        if (expr == "") return "";
        string res = ""; res += expr[0];
        for (int i = 1; i < (int)expr.size(); ++i) {
            //complete with catenation symbols where necessary
            if (
                (is_letter(expr[i - 1]) && is_letter(expr[i])) ||
                (is_letter(expr[i - 1]) && expr[i] == '(')     ||
                (expr[i - 1] == ')' && is_letter(expr[i]))     ||
                (expr[i - 1] == '*' && is_letter(expr[i]))     ||
                (expr[i - 1] == ')' && expr[i] == '(')         ||
                (expr[i - 1] == '*' && expr[i] == '(')
            )
                res += ".";
            res += expr[i];
        }
        res = "(" + res + ").#"; //catenate with end-marker
        return res;
    }

    bool is_letter(char ch) {return kLetters.find(ch) != kLetters.end() || ch == kLambda;}

    //deals with or-operator
    SyntaxTreeNode* expression_eval(const string& expr, int& curr_pos) {
        SyntaxTreeNode* root = new SyntaxTreeNode;
        auto son = term_eval(expr, curr_pos);
        root->first_pos = son->first_pos;
        root->last_pos = son->last_pos;
        root->sons.push_back(son);
        root->nullable = son->nullable;

        while (curr_pos < (int)expr.size() && expr[curr_pos] == '|') {
            son = term_eval(expr, ++curr_pos);
            root->sons.push_back(son);
            root->nullable = (root->nullable || son->nullable);
            for (int it : son->first_pos)
                root->first_pos.insert(it);
            for (int it : son->last_pos)
                root->last_pos.insert(it);
        }

        root->wipe_sons();
        return root;
    }

    //deals with cat-operator
    SyntaxTreeNode* term_eval(const string& expr, int& curr_pos) {
        SyntaxTreeNode* root = new SyntaxTreeNode;
        auto son = factor_eval(expr, curr_pos);
        root->first_pos = son->first_pos;
        root->last_pos = son->last_pos;
        root->sons.push_back(son);
        root->nullable = son->nullable;

        while (curr_pos < (int)expr.size() && expr[curr_pos] == '.') {
            son = factor_eval(expr, ++curr_pos);
            root->sons.push_back(son);
            for (int first : root->last_pos)
                for (int second : son->first_pos)
                    follow_pos_[first].insert(second);

            if (root->nullable)
                for (int it : son->first_pos)
                    root->first_pos.insert(it);
            if (son->nullable)
                for (int it : son->last_pos)
                    root->last_pos.insert(it);
            else
                root->last_pos = son->last_pos;
            root->nullable = (root->nullable && son->nullable);
        }

        root->wipe_sons();
        return root;
    }

    //deals with star-operator
    SyntaxTreeNode* factor_eval(const string& expr, int& curr_pos) {
        SyntaxTreeNode* root = new SyntaxTreeNode;
        if (expr[curr_pos] == '(') {
            curr_pos++;
            auto son = expression_eval(expr, curr_pos);
            root->first_pos = son->first_pos;
            root->last_pos = son->last_pos;
            root->sons.push_back(son);
            root->nullable = son->nullable;
            root->wipe_sons();
        }
        else {
            if (expr[curr_pos] == kLambda)
                root->nullable = true;
            else if (!is_letter(expr[curr_pos]) && expr[curr_pos] != '#')
                root->nullable = true, --curr_pos;
            else {
                ++leaves_count_;
                follow_pos_.resize((size_t)leaves_count_ + 1);
                what_letter_.resize((size_t)leaves_count_ + 1);
                root->nullable = false;
                root->first_pos = {leaves_count_};
                root->last_pos = {leaves_count_};
                root->leaf_index = leaves_count_;
                what_letter_[leaves_count_] = expr[curr_pos];
            }
        }

        if (++curr_pos < (int)expr.size() && expr[curr_pos] == '*') {
            root->nullable = true;
            for (int first : root->last_pos)
                for (int second : root->first_pos)
                    follow_pos_[first].insert(second);
        }
        while (curr_pos < (int)expr.size() && expr[curr_pos] == '*')
            curr_pos++;
        return root;
    }

    int leaves_count_;
    vector< unordered_set<int> > follow_pos_;
    vector<char> what_letter_;
    map< vector<int>, int > mark_;

    static const unordered_set<char> kLetters;
    static const char kLambda;
};

const unordered_set<char> RegexToDFAConverter::kLetters({
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
});
const char RegexToDFAConverter::kLambda = '$';

int main() {
    ifstream cin("data.in");
    ofstream cout("data.out");

    RegexToDFAConverter converter;

    string regex; cin >> regex;
    cout << converter.convert(regex);

    return 0;
}