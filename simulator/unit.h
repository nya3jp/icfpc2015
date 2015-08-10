#ifndef UNIT_H_
#define UNIT_H_

#include <vector>
#include <glog/logging.h>

#include "hexpoint.h"


class Unit {
 public:
  Unit(const HexPoint& pivot, std::vector<HexPoint>&& members);

  const std::vector<HexPoint>& members() const { return members_; }
  int order() const { return order_; }

  bool isEquivalent(const Unit& other) const;

 private:
  std::vector<HexPoint> members_;
  int order_;
};

class UnitLocation {
 public:
  class ConstMemberIter : public std::iterator<std::forward_iterator_tag,
                                               HexPoint> {
   public:
    ConstMemberIter(std::vector<HexPoint>::const_iterator iter,
                    HexPoint pivot,
                    int angle)
        : iter_(iter), pivot_(pivot), angle_(angle) {
    }

    HexPoint operator*() const {
      return iter_->RotateCounterClockwise(angle_).TranslateFromOrigin(pivot_);
    }

    bool operator==(const ConstMemberIter& other) const {
      DCHECK(pivot_ == other.pivot_);
      DCHECK(angle_ == other.angle_);
      return iter_ == other.iter_;
    }

    bool operator!=(const ConstMemberIter& other) const {
      return !(*this == other);
    }

    ConstMemberIter& operator++() {
      ++iter_;
      return *this;
    }

    ConstMemberIter operator++(int) {
      ConstMemberIter result(*this);
      ++iter_;
      return result;
    }

   private:
    std::vector<HexPoint>::const_iterator iter_;
    HexPoint pivot_;
    int angle_;
  };

  class Members {
   public:
    Members(const UnitLocation* unit) : unit_(unit) {
    }

    ConstMemberIter begin() const {
      return ConstMemberIter(unit_->unit_->members().begin(),
                             unit_->pivot_, unit_->angle_);
    }

    ConstMemberIter end() const {
      return ConstMemberIter(unit_->unit_->members().end(),
                             unit_->pivot_, unit_->angle_);
    }

    size_t size() const {
      return unit_->unit_->members().size();
    }
   private:
    const UnitLocation* unit_;
  };

  // Invalid data for convenience.
  UnitLocation() : unit_(nullptr), pivot_(0, 0), angle_(0) {
  }

  UnitLocation(const Unit* unit, const HexPoint& pivot)
      : unit_(unit), pivot_(pivot), angle_(0) {
  }

  UnitLocation(const Unit* unit, const HexPoint& pivot, int angle)
      : unit_(unit), pivot_(pivot), angle_(angle % unit->order()) {
  }

  const HexPoint& pivot() const { return pivot_; }
  int angle() const { return angle_; }
  Members members() const { return Members(this); }

  void Shift(int x) {
    pivot_ += HexPoint(x, 0);
  }

  void MoveEast() {
    Shift(1);
  }

  void MoveWest() {
    Shift(-1);
  }

  void MoveSouthEast() {
    pivot_ += HexPoint(pivot_.y() & 1, 1);
  }

  void MoveSouthWest() {
    pivot_ += HexPoint((pivot_.y() & 1) - 1, 1);
  }

  void RotateClockwise() {
    angle_ = (angle_ + 5) % unit_->order();
  }

  void RotateCounterClockwise() {
    angle_ = (angle_ + 1) % unit_->order();
  }

  bool operator==(const UnitLocation& other) const {
    return unit_ == other.unit_
        && pivot_ == other.pivot_
        && angle_ == other.angle_;
  }

  bool operator<(const UnitLocation& other) const {
    if (unit_ != other.unit_) return unit_ < other.unit_;
    if (angle_ != other.angle_) return angle_ < other.angle_;
    return HexPointLess()(pivot_, other.pivot_);
  }

  bool isEquivalent(const UnitLocation& other) const {
    return 
      (this->pivot_ == other.pivot_ &&
       this->angle_ == other.angle_ &&
       this->unit_->isEquivalent(*other.unit_));
  }

  struct Hash {
    typedef size_t result_type;
    result_type operator()(const UnitLocation& u) const {
       return reinterpret_cast<size_t>(u.unit_)
          ^ (u.pivot().x()<<8)
          ^ (u.pivot().y()<<17)
          ^ (u.angle()<<13);
    }
  };
  friend struct UnitLocation::Hash;

 private:
  const Unit* unit_;
  HexPoint pivot_;
  int angle_;
};

// Note: unit_ must be same.
struct UnitLocationLess {
  bool operator()(const UnitLocation& u1, const UnitLocation& u2) const {
    const HexPoint& p1 = u1.pivot();
    const HexPoint& p2 = u2.pivot();
    if (p1 != p2) {
      return HexPointLess()(p1, p2);
    }
    return u1.angle() < u2.angle();
  }
};

#endif  // UNIT_H_
