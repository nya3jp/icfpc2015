#ifndef GAME_H_
#define GAME_H_

#include <iostream>

#include "board.h"
#include "common.h"
#include "unit.h"
#include "rand_generator.h"

class Game {
 public:
  Game();
  ~Game();

  int score() const { return score_; }

  void Load(const picojson::value& parsed, int seed_index);
  void Dump(std::ostream* os) const;
  void DumpCurrent(std::ostream* os) const;

  // Find a new unit, and put it to the source.
  bool SpawnNewUnit();

  enum class Command {
    E, W, SE, SW, CW, CCW, IGNORED,
  };

  bool Run(Command action);

 private:
  int id_;
  std::vector<Unit> units_;
  Board board_;
  int source_length_;
  RandGenerator rand_;

  Unit current_unit_;
  int current_index_;
  std::vector<Unit> history_;
  int score_;
  int total_cleared_lines_;

  DISALLOW_COPY_AND_ASSIGN(Game);
};

std::ostream& operator<<(std::ostream& os, const Game& game);

#endif  // GAME_H_
