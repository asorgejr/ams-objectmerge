//
// Created by asorgejr on 7/14/2021.
//

#include "gtest/gtest.h"
#include "../src/ams_utils.h"

using namespace std;

#define TEST_SUITE_NAME test_hams


TEST(TEST_SUITE_NAME, Initialization) {
  // TODO
}

TEST(TEST_SUITE_NAME, InitializationWithString) {
  // TODO
}

TEST(TEST_SUITE_NAME, MemberInitialization) {
  // TODO
}

int main(int argc, char** argv) {
  int* argc_ = new int(argc);
  ::testing::InitGoogleTest(argc_, argv);
  return RUN_ALL_TESTS();
}
