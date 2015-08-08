#ifndef GAME_H_
#define GAME_H_

#include <iostream>
#include <glog/logging.h>

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

  static Unit NextUnit(const Unit& prev_unit, Command command);

  bool Run(Command action);

  Command GetLockCommand(const Unit& current) const;
  bool IsLockable(const Unit& current) const;

  typedef std::pair<Unit, std::vector<Command>> SearchResult;
  void ReachableUnits(std::vector<SearchResult>* result) const;

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
  int prev_cleared_lines_;
};

inline Game::Command operator++( Game::Command& x ) {
  return x = (Game::Command)(((int)(x) + 1));
}

std::ostream& operator<<(std::ostream& os, const Game& game);

inline std::ostream& operator<<(std::ostream& os, Game::Command command) {
  switch (command) {
    case Game::Command::W:
      os << "W";
      break;
    case Game::Command::E:
      os << "E";
      break;
    case Game::Command::SW:
      os << "SW";
      break;
    case Game::Command::SE:
      os << "SE";
      break;
    case Game::Command::CW:
      os << "CW";
      break;
    case Game::Command::CCW:
      os << "CCW";
      break;
    case Game::Command::IGNORED:
      os << "IGNORED";
      break;
    default:
      LOG(FATAL) << "Unknown command";
  }
  return os;
}

#endif  // GAME_H_
