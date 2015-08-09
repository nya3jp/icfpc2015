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

  void Shift(int x) {
    const HexPoint movement(x, 0);
    *this += movement;
  }

  void MoveEast() {
    Shift(1);
  }

  void MoveWest() {
    Shift(-1);
  }

  void MoveNorthEast() {
    const HexPoint odd_movement(1, -1);
    const HexPoint even_movement(0, -1);
    *this += (y() & 1 ? odd_movement : even_movement);
  }

  void MoveNorthWest() {
    const HexPoint odd_movement(0, -1);
    const HexPoint even_movement(-1, -1);
    *this += (y() & 1 ? odd_movement : even_movement);
  }

  void MoveSouthEast() {
    const HexPoint odd_movement(1, 1);
    const HexPoint even_movement(0, 1);
    *this += (y() & 1 ? odd_movement : even_movement);
  }

  void MoveSouthWest() {
    const HexPoint odd_movement(0, 1);
    const HexPoint even_movement(-1, 1);
    *this += (y() & 1 ? odd_movement : even_movement);
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
