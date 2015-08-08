#ifndef BOARD_H_
#define BOARD_H_

#include <iostream>
#include <vector>

#include <picojson.h>

#include "common.h"
#include "unit.h"

class Board {
 public:
  // TODO(hidehiko): more efficient implementation.
  typedef std::vector<std::vector<int> > Map;

  Board();
  ~Board();

  int width() const { return width_; }
  int height() const { return height_; }
  const Map& cells() const { return cells_; }

  void Load(const picojson::value& parsed);
  void Init(int width, int height);

  bool IsConflicting(const Unit& unit) const;
  int Lock(const Unit& unit);

  void Dump(std::ostream* os) const ;

 private:
  int width_;
  int height_;
  Map cells_;
};

std::ostream& operator<<(std::ostream& os, const Board& board);

#endif  // BOARD_H_
