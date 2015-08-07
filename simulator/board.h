#ifndef BOARD_H_
#define BOARD_H_

#include <iostream>
#include <vector>

#include <picojson.h>

#include "common.h"

class Board {
 public:
  Board();
  ~Board();

  void Load(const picojson::value& parsed);

  void Init(int width, int height);
  void Dump(std::ostream* os) const ;

 private:
  // TODO(hidehiko): more efficient implementation.
  typedef std::vector<std::vector<int> > Map;
  Map cells_;

  DISALLOW_COPY_AND_ASSIGN(Board);
};

std::ostream& operator<<(std::ostream& os, const Board& board);

#endif  // BOARD_H_
