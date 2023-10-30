//
// Created by asorgejr on 7/11/2021.
//

#include "ams_utils.h"
#include <regex>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>

using std::regex;
using std::regex_search;
using std::sregex_iterator;
using std::string;
using std::smatch;
using std::vector;
using std::map;
using std::cout;
using std::endl;
using std::stringstream;

namespace ams {

std::vector<OP_Node*> getInputAncestors(const OP_Node& node) {
  std::vector<OP_Node*> ret;
  OP_Node * upstream = node.getInput(0);
  if (!upstream) return ret;
  while (upstream) {
    ret.push_back(upstream);
    upstream = upstream->getInput(0);
  }
  return ret;
}

/** @brief represents a backreference in a replacement string. i.e. the '~1' in: "hello~1world!" */
class ExprObj {
public:
  /** @brief the backreference group number. i.e.: '1' in "~1".*/
  int group;
  /** @brief the position of the first character of the backreference expression in the replacement string.*/
  int position;
  /** @brief the size of the backreference expression.*/
  int size;
  ExprObj(int group, int position, int size) : group(group), position(position), size(size) {}
  ExprObj() = default;
  ~ExprObj() = default;
  std::string ToStdString() {
    stringstream ss;
    ss << "(" << group << ", " << position << ", " << size << ")";
    return ss.str();
  }
};

// TODO: trim the fat.
/**
 * @brief finds backreference expressions in a replacement string.
 * @param replace the replacement string to evaluate.
 * @param sorted if true, sort the map entries by backreference number.
 * @return a vector with the discovered backreference expressions.
 */
vector<ExprObj> findExprIndices(const string& replace, bool sorted=false) {
  const string expr = R"(~\d+)";
  regex rgx(expr);
  smatch m;
  bool result = regex_search(replace, m, rgx);
  auto ret = vector<ExprObj>();
  if (!result) return ret;
  
  // accumulate backreference expression matches.
  std::sregex_iterator next(replace.begin(), replace.end(), rgx);
  std::sregex_iterator end;
  while (next != end) {
    std::smatch match = *next;
    auto rawstr = match.str();
    string stri = "";
    for (int j = 1; j < rawstr.size(); j++) {
      stri += rawstr[j];
    }
    int num = atoi(stri.c_str());
    int pos = match.position();
    ret.push_back(ExprObj(num, pos, rawstr.size()));
    
    next++;
  }
  if (!sorted) {
    return ret;
  }
  
  // sort the vector by backref number.
  int i, j;
  ExprObj key;
  for (i = 1; i < ret.size(); i++) {
    key = ret[i];
    j = i - 1;
    while (j >= 0 && ret[j].group > key.group) {
      ret[j + 1] = ret[j];
      j = j - 1;
    }
    ret[j + 1] = key;
  }
  return ret;
}

std::string ams::re_replace(const string& str, const regex& rgx, const string& replace) {
  string result = str;
  sregex_iterator next(str.begin(), str.end(), rgx);
  sregex_iterator end;
  auto inds = findExprIndices(replace);
  bool hasGrpExpr = inds.size() > 0;
  
  if (hasGrpExpr)
    // backreference expressions require result to equal the replacement string instead of the input string.
    result = replace;
  
  // As the result string gets spliced/modified, its size will change.
  // This variable stores the difference so we can always index the correct position in the string.
  int offset = 0;

  while (next != end) {
    smatch match = *next;
    if (match.size() == 1) {
      // match does not contain backref groups
      string s = match.str();
      int pos = match.position()+offset;
      result = sslice(result, 0, pos-1) + replace + sslice(result, pos+s.size(), result.size()-1);
      offset += replace.size() - s.size();
    } else {
      // match contains backref groups
      for (auto expr : inds) {
        std::sub_match<string::const_iterator> grp;
        try {
          grp = match[expr.group];
          if (!grp.matched) continue;
        } catch (const std::exception& e) {
          cout << "invalid group expression in replacement string: " << expr.ToStdString() << endl;
          continue;
        }
        string s = grp.str();
        auto size = grp.length();
        auto pos = expr.position + offset;
        result = sslice(result, 0, pos-1) + s + sslice(result, pos + expr.size, result.size()-1);
        offset += s.size() - expr.size;
      }
    }
    next++;
  }
  return result;
}

string re_replace(const string& str, const string& pattern, const string& replace) {
  if (pattern.size() == 0) {
    return str;
  }
  regex rgx(pattern);
  return re_replace(str, rgx, replace);
}


std::string sslice(const string& str, size_t s, size_t e) {
  auto start = str.begin() + s;
  auto end = str.begin() + e + 1;
  std::string result(e - s + 1, ' ');
  copy(start, end, result.begin());
  return result;
}

}