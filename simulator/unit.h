#ifndef UNIT_H_
#define UNIT_H_

#include "hexpoint.h"

class Unit {
 public:
  Unit() : pivot_(-1, -1) {
  }

  Unit(const HexPoint& pivot, std::vector<HexPoint>&& members)
      : pivot_(pivot), members_(std::move(members)) {
  }

  const HexPoint& pivot() const { return pivot_; }
  const std::vector<HexPoint>& members() const { return members_; }

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
    for (auto& member : members_) {
      member = member.RotateClockwise(pivot_);
    }
  }

  void RotateCounterClockwise() {
    for (auto& member : members_) {
      member = member.RotateCounterClockwise(pivot_);
    }
  }

 private:
  HexPoint pivot_;
  std::vector<HexPoint> members_;
};

#endif  // UNIT_H_
