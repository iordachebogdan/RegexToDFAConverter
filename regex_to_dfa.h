#ifndef REGEXTODFACONVERTER_REGEX_TO_DFA_H
#define REGEXTODFACONVERTER_REGEX_TO_DFA_H

#include "dfa.h"
#include <unordered_set>
#include <string>
#include <vector>
#include "dfa.h"
using namespace std;

class RegexToDFAConverter {
 private:
  struct SyntaxTreeNode;
 public:
  RegexToDFAConverter() = default;
  RegexToDFAConverter(const RegexToDFAConverter&) = default;
  RegexToDFAConverter& operator=(const RegexToDFAConverter&) = default;
  virtual ~RegexToDFAConverter() = default;

  DFA convert(const string& expr);
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

  string simplify(const string& expr);

  bool is_letter(char ch);

  //deals with or-operator
  SyntaxTreeNode* expression_eval(const string& expr, int& curr_pos);
  //deals with cat-operator
  SyntaxTreeNode* term_eval(const string& expr, int& curr_pos);
  //deals with star-operator
  SyntaxTreeNode* factor_eval(const string& expr, int& curr_pos);

  int leaves_count_;
  vector< unordered_set<int> > follow_pos_;
  vector<char> what_letter_;
  map< vector<int>, int > mark_;

  static const unordered_set<char> kLetters;
  static const char kLambda;
};


#endif //REGEXTODFACONVERTER_REGEX_TO_DFA_H
