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

  void Load(const picojson::value& parsed);

  bool IsConflicting(const UnitLocation& unit) const;
  int Lock(const UnitLocation& unit);
  int LockPreview(const UnitLocation& unit) const;

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
