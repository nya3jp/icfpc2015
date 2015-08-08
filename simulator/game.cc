#include <algorithm>
#include <queue>
#include <glog/logging.h>

#include "game.h"

namespace {
// TODO refactor.
HexPoint ParseHexPoint(const picojson::value& value) {
  return HexPoint(
      value.get("x").get<int64_t>(), value.get("y").get<int64_t>());
}

std::ostream& operator<<(std::ostream& os,
                         const std::vector<HexPoint>& container) {
  bool first = true;
  os << "[";
  for (const auto& value : container) {
    if (first) {
      first = false;
    } else {
      os << ", ";
    }
    os << value;
  }
  os << "]";
  return os;
}

template<typename Container, typename T>
bool Contains(const Container& container, const T& value) {
  return std::find(std::begin(container), std::end(container), value) !=
      std::end(container);
}

}

Game::Game()
    : id_(-1), source_length_(-1), current_index_(-1), score_(-1) {
}

Game::~Game() {
}

void Game::Load(const picojson::value& parsed, int seed_index) {
  // Parse id.
  id_ = parsed.get("id").get<int64_t>();

  // Parse units.
  units_.clear();
  for (const picojson::value& value :
           parsed.get("units").get<picojson::array>()) {
    HexPoint pivot = ParseHexPoint(value.get("pivot"));
    std::vector<HexPoint> members;
    for (const picojson::value& m :
             value.get("members").get<picojson::array>()) {
      members.push_back(ParseHexPoint(m));
    }
    units_.emplace_back(pivot, std::move(members));
  }

  // Parse width, height and filled.
  board_.Load(parsed);

  // Parse sourceLength.
  source_length_ = parsed.get("sourceLength").get<int64_t>();

  // Parse sourceSeeds.
  rand_.set_seed(static_cast<uint32_t>(
      parsed.get("sourceSeeds").get(seed_index).get<int64_t>()));

  // Reset the current status.
  current_unit_ = Unit();
  current_index_ = 0;
  score_ = 0;
  prev_cleared_lines_ = 0;
  SpawnNewUnit();
}

bool Game::SpawnNewUnit() {
  if (current_index_ != 0) {
    rand_.Next();
  }
  ++current_index_;
  if (current_index_ > source_length_) {
    // Game over.
    return false;
  }

  current_unit_ = units_[rand_.current() % units_.size()];
  // Move to top.
  {
    HexPoint origin = current_unit_.members()[0];
    for (const auto& member : current_unit_.members()) {
      if (member.y() < origin.y()) {
        origin = member;
      }
    }
    for (auto& member : *current_unit_.mutable_members()) {
      member = member.TranslateToOrigin(origin);
    }
    *current_unit_.mutable_pivot() =
        current_unit_.pivot().TranslateToOrigin(origin);
  }

  // Center it.
  {
    int left = board_.width();
    int right = -1;
    for (const auto& p : current_unit_.members()) {
      if (left > p.x()) {
        left = p.x();
      }
      if (right < p.x()) {
        right = p.x();
      }
    }
    right = board_.width() - right - 1;
    current_unit_.Shift((right - left) / 2);
  }

  // Check if it is put to the available space.
  if (board_.IsConflicting(current_unit_)) {
    // No space left.
    return false;
  }

  // Clear the history.
  history_.clear();
  history_.push_back(current_unit_);
  return true;
}

Unit Game::NextUnit(const Unit& prev_unit, Command command) {
  // This won't happen.
  if (command == Command::IGNORED) {
    return prev_unit;
  }
  Unit new_unit = prev_unit;
  switch (command) {
    case Command::E: {
      new_unit.MoveEast();
      break;
    }
    case Command::W: {
      new_unit.MoveWest();
      break;
    }
    case Command::SE: {
      new_unit.MoveSouthEast();
      break;
    }
    case Command::SW: {
      new_unit.MoveSouthWest();
      break;
    }
    case Command::CW: {
      new_unit.RotateClockwise();
      break;
    }
    case Command::CCW: {
      new_unit.RotateCounterClockwise();
      break;
    }
  }
  return new_unit;
}

bool Game::Run(Command command) {
  if (command == Command::IGNORED) {
    return true;
  }
  Unit new_unit = Game::NextUnit(current_unit_, command);

  if (Contains(history_, new_unit)) {
    // Error.
    score_ = 0;
    return false;
  }

  if (board_.IsConflicting(new_unit)) {
    int num_cleard_lines = board_.Lock(current_unit_);
    int points = current_unit_.members().size()
        + 100 * (1 + num_cleard_lines) * num_cleard_lines / 2;
    int line_bonus = prev_cleared_lines_ > 0 ?
        (prev_cleared_lines_ - 1) * points / 10 : 0;
    prev_cleared_lines_ = num_cleard_lines;
    score_ += (points + line_bonus);
    return SpawnNewUnit();
  }

  current_unit_ = new_unit;
  history_.push_back(new_unit);
  return true;
}

Game::Command Game::GetLockCommand(const Unit& current) const {
  for (Command c = Command::E; c != Command::IGNORED; ++c) {
    Unit new_unit = Game::NextUnit(current, c);
    if (board_.IsConflicting(new_unit)) {
      return c;
    }
  }
  return Command::IGNORED;
}

bool Game::IsLockable(const Unit& current) const {
  return GetLockCommand(current) != Command::IGNORED;
}

void Game::ReachableUnits(std::vector<SearchResult>* result) const {
  result->clear();
  {
    Command c = GetLockCommand(current_unit_);
    if (c != Command::IGNORED) {
      result->emplace_back(SearchResult(current_unit_, {c}));
    }
  }

  std::queue<SearchResult> todo;
  todo.push(SearchResult(current_unit_, {}));
  std::vector<Unit> covered;
  covered.emplace_back(current_unit_);
  while (!todo.empty()) {
    Unit current = todo.front().first;
    std::vector<Command> moves = todo.front().second;
    todo.pop();
    for (Command c = Command::E; c != Command::IGNORED; ++c) {
      Unit next = Game::NextUnit(current, c);
      // TODO: performance improvement using set and such.
      if (Contains(covered, next)) {
        continue;
      }
      if (board_.IsConflicting(next)) {
        continue;
      }
      moves.emplace_back(c);
      todo.push(SearchResult(next, moves));
      covered.emplace_back(next);
      Command lock_command = GetLockCommand(next);
      if (lock_command != Command::IGNORED) {
        moves.emplace_back(lock_command);
        result->emplace_back(SearchResult(next, moves));
        moves.pop_back();
      }
      moves.pop_back();
    }
  }
  return;
}

void Game::Dump(std::ostream* os) const {
  *os << "ID: " << id_ << "\n";
  *os << "Units: \n";
  for (size_t i = 0; i < units_.size(); ++i) {
    *os << " [" << i << "]: " << units_[i].pivot() << ", "
        << units_[i].members() << "\n";
  }
  *os << "Board: \n" << board_ << "\n";
  *os << "SourceLength: " << source_length_ << "\n";
  *os << "RandSeed: " << rand_.seed();
}

void Game::DumpCurrent(std::ostream* os) const {
  *os << "current_index: " << current_index_ << "\n";
  *os << "Rand: " << rand_.current() << "\n";
  *os << "Score: " << score_ << "\n";

  *os << "Map:";
  {
    const Board::Map& cells = board_.cells();
    for (size_t y = 0; y < cells.size(); ++y) {
      *os << "\n";
      if (y & 1) {
        *os << ' ';
      }
      const std::vector<int>& row = cells[y];
      for (size_t x = 0; x < row.size(); ++x) {
        char c = '.';
        if (row[x]) {
          c = '*';
        }
        if (Contains(current_unit_.members(), HexPoint(x, y))) {
          c = '+';
        }
        if (current_unit_.pivot() == HexPoint(x, y)) {
          if (c == '+') {
            c = '@';
          } else {
            c = '%';
          }
        }
        if (x > 0) {
          *os << ' ';
        }
        *os << c;
      }
    }
  }
}

std::ostream& operator<<(std::ostream& os, const Game& game) {
  game.Dump(&os);
  return os;
}
