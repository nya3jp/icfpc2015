#ifndef UNIT_H_
#define UNIT_H_

#include "hexpoint.h"

class Unit {
 public:
  Unit() : pivot_(-1, -1) {
  }

  Unit(const HexPoint& pivot, std::vector<HexPoint>&& members)
      : pivot_(pivot), members_(std::move(members)), angle_(0) {
    std::sort(members_.begin(), members_.end(), HexPointLess());
  }

  const HexPoint& pivot() const { return pivot_; }
  const std::vector<HexPoint>& members() const { return members_; }
  int angle() const { return angle_; }

  HexPoint* mutable_pivot() { return &pivot_; }
  std::vector<HexPoint>* mutable_members() { return &members_; }

  bool operator==(const Unit& other) const {
    if (pivot_ != other.pivot_) {
      return false;
    }
    if (members_.size() != other.members_.size()) {
      return false;
    }
    for (size_t i = 0; i < members_.size(); ++i) {
      if (members_[i] != other.members_[i]) {
        return false;
      }
    }
    return true;
  }

  int GetTop() const {
    int top = std::numeric_limits<int>::max();
    for (auto& member : members_) {
      top = std::min(top, member.y());
    }
    return top;
  }

  int GetRight() const {
    int right = 0;
    for (auto& member : members_) {
      right = std::max(right, member.x());
    }
    return right;
  }

  int GetBottom() const {
    int bottom = 0;
    for (auto& member : members_) {
      bottom = std::max(bottom, member.y());
    }
    return bottom;
  }

  int GetLeft() const {
    int left = std::numeric_limits<int>::max();
    for (auto& member : members_) {
      left = std::min(left, member.x());
    }
    return left;
  }

  void Shift(int x) {
    const HexPoint movement(x, 0);
    pivot_ += movement;
    for (auto& member : members_) {
      member += movement;
    }
  }

  void MoveEast() {
    Shift(1);
  }

  void MoveWest() {
    Shift(-1);
  }

  void MoveSouthEast() {
    const HexPoint odd_movement(1, 1);
    const HexPoint even_movement(0, 1);
    pivot_ += (pivot_.y() & 1 ? odd_movement : even_movement);
    for (auto& member : members_) {
      member += (member.y() & 1 ? odd_movement : even_movement);
    }
  }

  void MoveSouthWest() {
    const HexPoint odd_movement(0, 1);
    const HexPoint even_movement(-1, 1);
    pivot_ += (pivot_.y() & 1 ? odd_movement : even_movement);
    for (auto& member : members_) {
      member += (member.y() & 1 ? odd_movement : even_movement);
    }
  }

  void RotateClockwise() {
    angle_ += 5;
    angle_ %= 6;
    for (auto& member : members_) {
      member = member.RotateClockwise(pivot_);
    }
    std::sort(members_.begin(), members_.end(), HexPointLess());
  }

  void RotateCounterClockwise() {
    angle_ ++;
    angle_ %= 6;
    for (auto& member : members_) {
      member = member.RotateCounterClockwise(pivot_);
    }
    std::sort(members_.begin(), members_.end(), HexPointLess());
  }

 private:
  struct HexPointLess {
    bool operator()(const HexPoint& p1, const HexPoint& p2) const {
      return p1.y() != p2.y() ? p1.y() < p2.y() : p1.x() < p2.x();
    }
  };

  HexPoint pivot_;
  std::vector<HexPoint> members_;
  int angle_;
};

#endif  // UNIT_H_
