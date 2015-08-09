#include <algorithm>

#include "unit.h"

namespace {

// Member must be sorted in HexPointLess order. Also, this assumes the pivot
// of the rotation is origin.
int GetOrder(const std::vector<HexPoint>& member) {
  std::vector<HexPoint> moved(member);

  // The order is 1, 2, 3, or 6.
  for (int i = 0; i < 3; ++i) {
    for (auto& point : moved) {
      point = point.RotateCounterClockwise();
    }
    std::sort(moved.begin(), moved.end(), HexPointLess());
    if (std::equal(member.begin(), member.end(), moved.begin())) {
      return i + 1;
    }
  }
  return 6;
}

}  // namespace

Unit::Unit(const HexPoint& pivot, std::vector<HexPoint>&& members) {
  // First move pivot to the origin.
  for (auto& member : members) {
    member = member.TranslateToOrigin(pivot);
  }
  // Then sort for stabilization.
  std::sort(members.begin(), members.end(), HexPointLess());
  members_ = std::move(members);

  // Also pre-compute the order.
  order_ = GetOrder(members_);
}
