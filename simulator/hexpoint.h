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

  void RotClockwise();
  void RotCounterClockwize();

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

  HexPoint RotateClockwise() const {
    int xx = x_ - y_ / 2;
    int zz = y_;
    int yy = -xx - zz;

    int tmp = xx;
    xx = -zz;
    zz = -yy;
    yy = -tmp;

    return HexPoint(xx + zz / 2, zz);
  }

  HexPoint RotateCounterClockwise() const {
    int xx = x_ - y_ / 2;
    int zz = y_;
    int yy = -xx - zz;

    int tmp = xx;
    xx = -yy;
    yy = -zz;
    zz = -tmp;

    return HexPoint(xx + zz / 2, zz);
  }

  HexPoint RotateClockwise(const HexPoint& origin) const {
    return (*this - origin).RotateClockwise() + origin;
  }

  HexPoint RotateCounterClockwise(const HexPoint& origin) const {
    return (*this - origin).RotateCounterClockwise() + origin;
  }

 private:
  int x_;
  int y_;
};

inline std::ostream& operator<<(std::ostream& os, const HexPoint& p) {
  return os << "(" << p.x() << ", " << p.y() << ")";
}

#endif  // HEXPOIONT_H_
