//
// Created by asorgejr on 7/14/2021.
//

#include "../src/ams_utils.h"
#include <iostream>


using namespace std;

bool scompare(const string& subject, const string& expected) {
  if (subject.size() != expected.size()) return false;
  for (int i = 0; i < expected.size(); i++) {
    if (subject[i] != expected[i]) return false;
  }
  return true;
}

bool test_re_replace(string subject, string pattern, string replace, string expected) {
  string result = ams::re_replace(subject, pattern, replace);
  assert(scompare(result, expected));
  return true;
}


int main(int argc, char *argv[]) {
  const string subject = "/obj/node/null/geo";
  bool test0 = test_re_replace(subject, "null", "parent", "/obj/node/parent/geo");
  bool test1 = test_re_replace(subject, "(/obj/node)/null(/geo)", "~2~1", "/geo/obj/node");
  bool test2 = test_re_replace(subject+"/null", "null", "parent", "/obj/node/parent/geo/parent");
  bool err0 = test_re_replace(subject, "(/obj/node)/null(/geo)", "~3~1", "~3/obj/node");
  bool test3 = test_re_replace(subject, "(/obj/node)(/null)(/geo)", "~2~3~1", "/null/geo/obj/node");
  return 0;
}
