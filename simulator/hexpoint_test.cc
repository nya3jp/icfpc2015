#include <gtest/gtest.h>

#include "hexpoint.h"

TEST(HexPoint, Rotate) {
  // Random typical use case.
  EXPECT_EQ(HexPoint(1, 2), HexPoint(2, 0).RotateClockwise());
  EXPECT_EQ(HexPoint(2, 0), HexPoint(1, 2).RotateCounterClockwise());
  EXPECT_EQ(HexPoint(2, 7), HexPoint(4, 5).RotateClockwise(HexPoint(2, 4)));
  EXPECT_EQ(HexPoint(2, 6), HexPoint(4, 5).RotateClockwise(HexPoint(2, 3)));
  EXPECT_EQ(HexPoint(-1, 3), HexPoint(2, 2).RotateClockwise());
  EXPECT_EQ(HexPoint(-1, 3), HexPoint(2, 2).RotateClockwise());
  EXPECT_EQ(HexPoint(0, -1), HexPoint(-1, -1).RotateClockwise());

  // Test for even row.
  // Distance 1, centering (2, 2).
  EXPECT_EQ(HexPoint(2, 1), HexPoint(1, 1).RotateClockwise(HexPoint(2, 2)));
  EXPECT_EQ(HexPoint(3, 2), HexPoint(2, 1).RotateClockwise(HexPoint(2, 2)));
  EXPECT_EQ(HexPoint(2, 3), HexPoint(3, 2).RotateClockwise(HexPoint(2, 2)));
  EXPECT_EQ(HexPoint(1, 3), HexPoint(2, 3).RotateClockwise(HexPoint(2, 2)));
  EXPECT_EQ(HexPoint(1, 2), HexPoint(1, 3).RotateClockwise(HexPoint(2, 2)));
  EXPECT_EQ(HexPoint(1, 1), HexPoint(1, 2).RotateClockwise(HexPoint(2, 2)));

  // Distance 2-1, centering (2, 2).
  EXPECT_EQ(HexPoint(3, 0), HexPoint(1, 0).RotateClockwise(HexPoint(2, 2)));
  EXPECT_EQ(HexPoint(4, 2), HexPoint(3, 0).RotateClockwise(HexPoint(2, 2)));
  EXPECT_EQ(HexPoint(3, 4), HexPoint(4, 2).RotateClockwise(HexPoint(2, 2)));
  EXPECT_EQ(HexPoint(1, 4), HexPoint(3, 4).RotateClockwise(HexPoint(2, 2)));
  EXPECT_EQ(HexPoint(0, 2), HexPoint(1, 4).RotateClockwise(HexPoint(2, 2)));
  EXPECT_EQ(HexPoint(1, 0), HexPoint(0, 2).RotateClockwise(HexPoint(2, 2)));

  // Distance 2-2, centering (2, 2).
  EXPECT_EQ(HexPoint(3, 1), HexPoint(2, 0).RotateClockwise(HexPoint(2, 2)));
  EXPECT_EQ(HexPoint(3, 3), HexPoint(3, 1).RotateClockwise(HexPoint(2, 2)));
  EXPECT_EQ(HexPoint(2, 4), HexPoint(3, 3).RotateClockwise(HexPoint(2, 2)));
  EXPECT_EQ(HexPoint(0, 3), HexPoint(2, 4).RotateClockwise(HexPoint(2, 2)));
  EXPECT_EQ(HexPoint(0, 1), HexPoint(0, 3).RotateClockwise(HexPoint(2, 2)));
  EXPECT_EQ(HexPoint(2, 0), HexPoint(0, 1).RotateClockwise(HexPoint(2, 2)));

  // Test for odd row.
  // Distance 1, centering (2, 3).
  EXPECT_EQ(HexPoint(3, 2), HexPoint(2, 2).RotateClockwise(HexPoint(2, 3)));
  EXPECT_EQ(HexPoint(3, 3), HexPoint(3, 2).RotateClockwise(HexPoint(2, 3)));
  EXPECT_EQ(HexPoint(3, 4), HexPoint(3, 3).RotateClockwise(HexPoint(2, 3)));
  EXPECT_EQ(HexPoint(2, 4), HexPoint(3, 4).RotateClockwise(HexPoint(2, 3)));
  EXPECT_EQ(HexPoint(1, 3), HexPoint(2, 4).RotateClockwise(HexPoint(2, 3)));
  EXPECT_EQ(HexPoint(2, 2), HexPoint(1, 3).RotateClockwise(HexPoint(2, 3)));

  // Distance 2-1, centering (2, 3).
  EXPECT_EQ(HexPoint(3, 1), HexPoint(1, 1).RotateClockwise(HexPoint(2, 3)));
  EXPECT_EQ(HexPoint(4, 3), HexPoint(3, 1).RotateClockwise(HexPoint(2, 3)));
  EXPECT_EQ(HexPoint(3, 5), HexPoint(4, 3).RotateClockwise(HexPoint(2, 3)));
  EXPECT_EQ(HexPoint(1, 5), HexPoint(3, 5).RotateClockwise(HexPoint(2, 3)));
  EXPECT_EQ(HexPoint(0, 3), HexPoint(1, 5).RotateClockwise(HexPoint(2, 3)));
  EXPECT_EQ(HexPoint(1, 1), HexPoint(0, 3).RotateClockwise(HexPoint(2, 3)));

  // Distance 2-2, centering (2, 3).
  EXPECT_EQ(HexPoint(4, 2), HexPoint(2, 1).RotateClockwise(HexPoint(2, 3)));
  EXPECT_EQ(HexPoint(4, 4), HexPoint(4, 2).RotateClockwise(HexPoint(2, 3)));
  EXPECT_EQ(HexPoint(2, 5), HexPoint(4, 4).RotateClockwise(HexPoint(2, 3)));
  EXPECT_EQ(HexPoint(1, 4), HexPoint(2, 5).RotateClockwise(HexPoint(2, 3)));
  EXPECT_EQ(HexPoint(1, 2), HexPoint(1, 4).RotateClockwise(HexPoint(2, 3)));
  EXPECT_EQ(HexPoint(2, 1), HexPoint(1, 2).RotateClockwise(HexPoint(2, 3)));

  // TODO: CCW test.
}
