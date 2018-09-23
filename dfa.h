#ifndef REGEXTODFACONVERTER_DFA_H
#define REGEXTODFACONVERTER_DFA_H

#include <iostream>
#include <unordered_set>
#include <map>
using namespace std;

class DFA {
 public:
  DFA() = default;
  DFA(const DFA&) = default;
  DFA& operator=(const DFA&) = default;
  virtual ~DFA() = default;

  int size() const;
  void add_node(int node);

  void set_initial(int node);
  void add_final(int node);

  void add_transition(int from, char letter, int to);

  friend ostream& operator << (ostream& out, const DFA& dfa);
 private:
  unordered_set<int> nodes_;
  unordered_set<int> final_nodes_;
  int initial_;
  map<pair<int, char>, int> transitions_;
};

#endif //REGEXTODFACONVERTER_DFA_H
