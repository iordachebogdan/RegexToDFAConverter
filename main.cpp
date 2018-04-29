#include <iostream>
#include <map>
#include <unordered_set>
using namespace std;

class DFA {
  public:
    DFA() = default;
    DFA(const DFA&) = default;
    DFA& operator=(const DFA&) = default;

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

int main() {

    return 0;
}