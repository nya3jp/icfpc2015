#include <algorithm>

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
    : id_(-1), source_length_(-1), current_index_(-1) {
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
  if (board_.IsConflicting(current_unit_)) {
    // No space left.
    return false;
  }

  history_.clear();
  history_.push_back(current_unit_);
  return true;
}

bool Game::Run(Command command) {
  Unit new_unit = current_unit_;
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

  if (Contains(history_, new_unit)) {
    // Error.
    // TODO score = 0;
    return false;
  }

  if (board_.IsConflicting(new_unit)) {
    board_.Lock(current_unit_);
    return SpawnNewUnit();
  }

  current_unit_ = new_unit;
  history_.push_back(new_unit);
  return true;
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
