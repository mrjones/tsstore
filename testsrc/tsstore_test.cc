#include "gtest/gtest.h"
#include "../src/tsstore.h"

TEST(Foo, Bar) {
  TSStore store;
  ASSERT_EQ("foo", store.foo());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
