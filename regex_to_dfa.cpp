#include "regex_to_dfa.h"
#include <algorithm>
#include <queue>
using namespace std;

const unordered_set<char> RegexToDFAConverter::kLetters({
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
});
const char RegexToDFAConverter::kLambda = '$';

DFA RegexToDFAConverter::convert(const string &expr) {
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

string RegexToDFAConverter::simplify(const string &expr) {
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

bool RegexToDFAConverter::is_letter(char ch) {
  return kLetters.find(ch) != kLetters.end() || ch == kLambda;
}

RegexToDFAConverter::SyntaxTreeNode* RegexToDFAConverter::expression_eval(
    const string &expr, int &curr_pos) {

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

RegexToDFAConverter::SyntaxTreeNode* RegexToDFAConverter::term_eval(
    const string &expr, int &curr_pos) {

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

RegexToDFAConverter::SyntaxTreeNode* RegexToDFAConverter::factor_eval(
    const string &expr, int &curr_pos) {

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