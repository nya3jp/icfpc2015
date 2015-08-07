#include <gtest/gtest.h>

#include "hexpoint.h"

TEST(HexPoint, Rotate) {
  EXPECT_EQ(HexPoint(1, 2), HexPoint(2, 0).RotateClockwise());
  EXPECT_EQ(HexPoint(2, 0), HexPoint(1, 2).RotateCounterClockwise());

  EXPECT_EQ(HexPoint(2, 7), HexPoint(4, 5).RotateClockwise(HexPoint(2, 4)));
  EXPECT_EQ(HexPoint(2, 6), HexPoint(4, 5).RotateClockwise(HexPoint(2, 3)));

  EXPECT_EQ(HexPoint(-1, 3), HexPoint(2, 2).RotateClockwise());
  // TODO more tests...
}
