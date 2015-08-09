#ifndef BOARD_H_
#define BOARD_H_

#include <iostream>
#include <vector>

#include <picojson.h>

#include "common.h"
#include "unit.h"

class Board {
 public:
  Board();
  ~Board();

  int width() const { return width_; }
  int height() const { return height_; }
  bool operator()(int x, int y) const {
    return cells_[y * width_ + x];
  }
  bool operator()(const HexPoint &h) const {
    return this->operator()(h.x(), h.y());
  }

  void Set(int x, int y, bool value) {
    cells_[y * width_ + x] = value;
  }
  void Set(const HexPoint &h, bool value) {
    this->Set(h.x(), h.y(), value);
  }

  void Load(const picojson::value& parsed);

  bool IsConflicting(const Unit& unit) const;
  int Lock(const Unit& unit);
  int LockPreview(const Unit& unit) const;

  void Dump(std::ostream* os) const;

 private:
  typedef std::vector<bool> Map;
  int width_;
  int height_;
  Map cells_;
};

inline std::ostream& operator<<(std::ostream& os, const Board& board) {
  board.Dump(&os);
  return os;
}

#endif  // BOARD_H_
