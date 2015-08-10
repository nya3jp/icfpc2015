#ifndef GAME_H_
#define GAME_H_

#include <iostream>
#include <vector>
#include <glog/logging.h>

#include "board.h"
#include "common.h"
#include "unit.h"
#include "rand_generator.h"

struct Bound {
  int top, bottom, left, right;
};

class GameData {
 public:
  GameData();

  int id() const { return id_; }
  const std::vector<Unit>& units() const { return units_; }
  const std::vector<HexPoint>& spawn_position() const {
    return spawn_position_;
  }
  const Board& board() const { return board_; }
  int source_length() const { return source_length_; }
  const std::vector<int>& source_seeds() const { return source_seeds_; }

  const std::vector<Bound>& unit_pivot_bounds() const {
    return unit_pivot_bounds_;
  }

  void Load(const picojson::value& parsed);
  void Dump(std::ostream* os) const;

 private:
  int id_;
  // TODO: do not copy units_.
  std::vector<Unit> units_;
  std::vector<HexPoint> spawn_position_;
  Board board_;
  int source_length_;
  std::vector<int> source_seeds_;

  std::vector<Bound> unit_pivot_bounds_;
};

inline std::ostream& operator<<(std::ostream& os, const GameData& data) {
  data.Dump(&os);
  return os;
}

class Game {
 public:
  Game();
  ~Game();

  int score() const { return score_; }
  int current_index() const { return current_index_; }
  int units_remaining() const { return data_->source_length() - current_index_; }
  bool is_finished() const { return is_finished_; }
  bool error() const { return error_; }

  void Init(const GameData* data, int rand_seed_index);
  void Dump(std::ostream* os) const;

  const Board& GetBoard() const { return board_; }

  // Find a new unit, and put it to the source.
  bool SpawnNewUnit();

  enum class Command {
    E, W, SE, SW, CW, CCW, IGNORED,
  };

  static const char command_char_map_[7][7];

  static Command Char2Command(char code);
  static const char* Command2Chars(Command com);
  // Converts command sequence to string, ignoring power phrase stuff.
  static std::string Commands2SimpleString(
      const std::vector<Command>& commands);

  // Given the current position and the op command, returns the next position.
  static UnitLocation NextUnit(const UnitLocation& prev_unit,
                               Command command);

  bool Run(Command action);
  bool RunSequence(const std::vector<Command>& actions);

  // Given the current unit position, returns a command to lock the unit
  // at the position, or returns IGNORED if it's impossible to lock it.
  Command GetLockCommand(const UnitLocation& current) const;
  // Returns whether the unit is lockable at the given position.
  bool IsLockable(const UnitLocation& current) const;
  // Returns whether it is lockable by the given command.
  bool IsLockableBy(const UnitLocation& current, Command cmd) const;

  typedef std::pair<UnitLocation, std::vector<Command>> SearchResult;
  // Does BFS search from current_unit_ to return the list of lockable locations
  // with the command sequence to reach there.
  void ReachableUnits(std::vector<SearchResult>* result) const;
  const Board& board() const { return board_; }

  const UnitLocation& current_unit() const { return current_unit_; }

  UnitLocation GetUnitAtSpawnPosition(size_t index) const {
    return UnitLocation(&data_->units()[index],
                        data_->spawn_position()[index]);
  }
  size_t GetNumberOfUnits() const { return data_->units().size(); }
  const std::vector<Unit>& units() const { return data_->units(); }

  const int prev_cleared_lines() const { return prev_cleared_lines_; }

 private:
  const GameData* data_;
  Board board_;
  RandGenerator rand_;
  UnitLocation current_unit_;
  int current_index_;
  std::vector<UnitLocation> history_;
  int score_;
  int prev_cleared_lines_;
  bool is_finished_;
  bool error_;
};

inline Game::Command operator++( Game::Command& x ) {
  return x = (Game::Command)(((int)(x) + 1));
}

inline std::ostream& operator<<(std::ostream& os, const Game& game) {
  game.Dump(&os);
  return os;
}


struct CurrentState {
  CurrentState(const Game& game) : game_(game) {
  }

  const Game& game_;
};

inline std::ostream& operator<<(std::ostream& os, const CurrentState& state) {
  state.game_.Dump(&os);
  return os;
}

inline Game::Command ParseCommand(char c) {
  switch(c) {
    case 'p': case '\'': case '!': case '.': case '0': case '3':
      return Game::Command::W;
    case 'b': case 'c': case 'e': case 'f': case 'y': case '2':
      return Game::Command::E;
    case 'a': case 'g': case 'h': case 'i': case 'j': case '4':
      return Game::Command::SW;
    case 'l': case 'm': case 'n': case 'o': case ' ': case '5':
      return Game::Command::SE;
    case 'd': case 'q': case 'r': case 'v': case 'z': case '1':
      return Game::Command::CW;
    case 'k': case 's': case 't': case 'u': case 'w': case 'x':
      return Game::Command::CCW;
    case '\t': case '\n': case '\r':
      return Game::Command::IGNORED;
    defult:
      LOG(FATAL) << "Unknown Character: " << c << "(" << (int) c << ")";
  }
}

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

inline Game::Command Game::Char2Command(char code) {
  return ParseCommand(code);
}

#endif  // GAME_H_
