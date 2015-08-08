#ifndef HEXPOINT_H_
#define HEXPOINT_H_

#include <iostream>

class HexPoint {
 public:
  HexPoint() : x_(0), y_(0) {
  }

  HexPoint(int x, int y) : x_(x), y_(y) {
  }

  int x() const { return x_; }
  int y() const { return y_; }

  HexPoint operator+(const HexPoint& other) const {
    return HexPoint(x_ + other.x_, y_ + other.y_);
  }
  HexPoint operator-(const HexPoint& other) const {
    return HexPoint(x_ - other.x_, y_ - other.y_);
  }

  HexPoint& operator+=(const HexPoint& other) {
    x_ += other.x_;
    y_ += other.y_;
    return *this;
  }

  HexPoint& operator-=(const HexPoint& other) {
    x_ -= other.x_;
    y_ -= other.y_;
    return *this;
  }

  bool operator==(const HexPoint& other) const {
    return x_ == other.x_ && y_ == other.y_;
  }
  bool operator!=(const HexPoint& other) const {
    return !(*this == other);
  }

  // Rotate 60 degrees in clockwise centering (0, 0).
  HexPoint RotateClockwise() const {
    int xx = x_ - (y_ - (y_ & 1)) / 2;
    int zz = y_;
    int yy = -xx - zz;

    int tmp = xx;
    xx = -zz;
    zz = -yy;
    yy = -tmp;

    return HexPoint(xx + (zz - (zz & 1)) / 2, zz);
  }

  // Rotate 60 degrees in counter-clockwise centering (0, 0).
  HexPoint RotateCounterClockwise() const {
    int xx = x_ - (y_ - (y_ & 1)) / 2;
    int zz = y_;
    int yy = -xx - zz;

    int tmp = xx;
    xx = -yy;
    yy = -zz;
    zz = -tmp;

    return HexPoint(xx + (zz - (zz & 1)) / 2, zz);
  }

  // Translate this point as if |pivot| moves to (0, 0).
  HexPoint TranslateToOrigin(const HexPoint& pivot) const {
    HexPoint moved = *this - pivot;
    moved.x_ -= (pivot.y() & moved.y()) & 1;
    return moved;
  }

  // Translate this point as if (0, 0) moves to |pivot|.
  HexPoint TranslateFromOrigin(const HexPoint& pivot) const {
    HexPoint moved = *this + pivot;
    moved.x_ += (pivot.y() & y()) & 1;
    return moved;
  }

  // Rotate 60 degrees in clockwise, centering pivot.
  HexPoint RotateClockwise(const HexPoint& pivot) const {
    return TranslateToOrigin(pivot)
        .RotateClockwise()
        .TranslateFromOrigin(pivot);
  }

  // Rotate 60 degrees in counter-clockwise, centering pivot.
  HexPoint RotateCounterClockwise(const HexPoint& pivot) const {
    return TranslateToOrigin(pivot)
        .RotateCounterClockwise()
        .TranslateFromOrigin(pivot);
  }

 private:
  int x_;
  int y_;
};

inline std::ostream& operator<<(std::ostream& os, const HexPoint& p) {
  return os << "(" << p.x() << ", " << p.y() << ")";
}

#endif  // HEXPOIONT_H_
