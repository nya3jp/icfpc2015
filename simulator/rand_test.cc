#include <gtest/gtest.h>

#include "rand_generator.h"

TEST(RandGeneratorTest, Sample) {
  RandGenerator r;
  r.set_seed(17);

  EXPECT_EQ(0, r.current());
  r.Next();
  EXPECT_EQ(24107, r.current());
  r.Next();
  EXPECT_EQ(16552, r.current());
  r.Next();
  EXPECT_EQ(12125, r.current());
  r.Next();
  EXPECT_EQ(9427, r.current());
  r.Next();
  EXPECT_EQ(13152, r.current());
  r.Next();
  EXPECT_EQ(21440, r.current());
  r.Next();
  EXPECT_EQ(3383, r.current());
  r.Next();
  EXPECT_EQ(6873, r.current());
  r.Next();
  EXPECT_EQ(16117, r.current());
}
