#include <algorithm>
#include <queue>
#include <set>
#include <glog/logging.h>

#include "game.h"
#include "scorer.h"

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

std::ostream& operator<<(std::ostream& os,
                         const std::vector<int>& container) {
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

// Find Spawn position.
HexPoint GetSpawnPosition(const Unit& unit, int width) {
  // First, find the top most position.
  HexPoint top = unit.members()[0];
  {
    for (const auto& member : unit.members()) {
      if (member.y() < top.y()) {
        top = member;
      }
    }
  }

  // Move as if top moves to the origin.
  UnitLocation location(&unit, HexPoint(0, 0).TranslateToOrigin(top));

  // Then adjust horizontal position.
  int left = width;
  int right = -1;
  for (const auto& member : location.members()) {
    left = std::min(left, member.x());
    right = std::max(right, member.x());
  }
  location.Shift(((width - right - 1) - left) / 2);

  return location.pivot();
}

Bound GetPivotApploxBound(const Unit& unit, int width, int height) {
  Bound result = { std::numeric_limits<int>::max(),
                   std::numeric_limits<int>::min(),
                   std::numeric_limits<int>::max(),
                   std::numeric_limits<int>::min() };
  for (int angle = 0; angle < unit.order(); ++angle) {
    UnitLocation base(&unit, HexPoint(0, 0), angle);

    // Top.
    {
      HexPoint top(0, std::numeric_limits<int>::max());
      for (const auto& member : base.members()) {
        if (top.y() > member.y()) {
          top = member;
        }
      }

      {
        UnitLocation location(
            &unit, HexPoint(0, 0).TranslateToOrigin(top), angle);

        result.top = std::min(result.top, location.pivot().y());
        result.bottom = std::max(result.bottom, location.pivot().y());
        int left = std::numeric_limits<int>::max();
        int right = std::numeric_limits<int>::min();
        for (const auto& member : location.members()) {
          left = std::min(left, member.x());
          right = std::max(right, member.x());
        }
        result.left = std::min(result.left, location.pivot().x() - left);
        result.right = std::max(
            result.right, location.pivot().x() + (width - right - 1));
      }
    }

    // Bottom.
    {
      HexPoint bottom(0, std::numeric_limits<int>::min());
      for (const auto& member : base.members()) {
        if (bottom.y() < member.y()) {
          bottom = member;
        }
      }

      {
        UnitLocation location(
            &unit,
            HexPoint(0, 0)
                .TranslateToOrigin(bottom)
                .TranslateFromOrigin(HexPoint(0, height - 1)),
            angle);

        result.top = std::min(result.top, location.pivot().y());
        result.bottom = std::max(result.bottom, location.pivot().y());
        int left = std::numeric_limits<int>::max();
        int right = std::numeric_limits<int>::min();
        for (const auto& member : location.members()) {
          left = std::min(left, member.x());
          right = std::max(right, member.x());
        }
        result.left = std::min(result.left, location.pivot().x() - left);
        result.right = std::max(
            result.right, location.pivot().x() + (width - right - 1));
      }
    }
  }

  // Hack for hex cell.
  result.left -= 1;
  result.right += 1;
  return result;
}

}

GameData::GameData() : id_(-1), source_length_(-1) {
}

void GameData::Load(const picojson::value& parsed) {
  // Parse id.
  id_ = parsed.get("id").get<int64_t>();

  // Parse width, height and filled.
  board_.Load(parsed);

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
    spawn_position_.push_back(GetSpawnPosition(units_.back(), board_.width()));
    unit_pivot_bounds_.push_back(GetPivotApploxBound(
        units_.back(), board_.width(), board_.height()));
  }

  // Parse sourceLength.
  source_length_ = parsed.get("sourceLength").get<int64_t>();

  // Parse sourceSeeds.
  source_seeds_.clear();
  {
    const picojson::array& source_seeds =
        parsed.get("sourceSeeds").get<picojson::array>();
    for (const picojson::value& seed : source_seeds) {
      source_seeds_.push_back(seed.get<int64_t>());
    }
  }
}

void GameData::Dump(std::ostream* os) const {
  *os << "ID: " << id_ << "\n";
  *os << "Units: \n";
  for (size_t i = 0; i < units_.size(); ++i) {
    *os << " [" << i << "]: " << units_[i].members() << ", "
        << units_[i].order() << ", "
        << spawn_position_[i] << "\n";
  }

  *os << "Board: \n" << board_ << "\n";
  *os << "SourceLength: " << source_length_ << "\n";
  *os << "SourceSeeds: " << source_seeds_;
}


Game::Game() :  current_index_(-1), score_(-1) {
}

Game::~Game() {
}

void Game::Init(const GameData* data, int rand_seed_index) {
  data_ = data;
  board_ = data_->board();
  // Parse sourceSeeds.
  rand_.set_seed(data_->source_seeds()[rand_seed_index]);

  // Reset the current status.
  current_unit_ = UnitLocation();
  current_index_ = 0;
  history_.clear();
  score_ = 0;
  prev_cleared_lines_ = 0;
  is_finished_ = false;
  error_ = false;
  SpawnNewUnit();
}

bool Game::SpawnNewUnit() {
  if (current_index_ != 0) {
    rand_.Next();
  }
  ++current_index_;
  if (current_index_ > data_->source_length()) {
    // All units are used.
    is_finished_ = true;
    return false;
  }

  int next_index = rand_.current() % data_->units().size();
  current_unit_ = GetUnitAtSpawnPosition(next_index);

  // Check if it is put to the available space.
  if (board_.IsConflicting(current_unit_)) {
    // New unit conclicts with board.
    is_finished_ = true;
    return false;
  }

  // Clear the history.
  history_.clear();
  history_.push_back(current_unit_);
  return true;
}

const char Game::command_char_map_[7][7] = {
  "ebcfy2",  // E
  "!p'.03",  // W
  " lmno5",  // SE
  "iaghj4",  // SW
  "dqrvz1",  // CW
  "kstuwx",  // CCW
  "\t\n\r\t\n\r"  // ignored
};

const char* Game::Command2Chars(Command com) {
  return Game::command_char_map_[(int)com];
}

std::string Game::Commands2SimpleString(const std::vector<Command>& commands) {
  std::string result;
  result.reserve(commands.size() + 1);
  for (const auto& c : commands) {
    result += Command2Chars(c)[0];
  }
  return result;
}


UnitLocation Game::NextUnit(const UnitLocation& prev_unit, Command command) {
  // This won't happen.
  if (command == Command::IGNORED) {
    return prev_unit;
  }
  UnitLocation new_unit = prev_unit;
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
  if (error_) {
    // If the state is already in error, do nothing.
    return false;
  }
  if (is_finished_) {
    // Running a step after the finish, causes an error.
    error_ = true;
    score_ = 0;
    return false;
  }

  if (command == Command::IGNORED) {
    return true;
  }
  UnitLocation new_unit = Game::NextUnit(current_unit_, command);

  if (Contains(history_, new_unit)) {
    // Error.
    is_finished_ = true;
    error_ = true;
    score_ = 0;
    return false;
  }

  if (board_.IsConflicting(new_unit)) {
    int num_cleared_lines = board_.Lock(current_unit_);
    score_ += MoveScore(current_unit_.members().size(),
                        num_cleared_lines, prev_cleared_lines_);
    prev_cleared_lines_ = num_cleared_lines;
    return SpawnNewUnit();
  }

  current_unit_ = new_unit;
  history_.push_back(new_unit);
  return true;
}

bool Game::RunSequence(const std::vector<Command>& commands) {
  for (const auto& c : commands) {
    if (!Run(c))
      return false;
  }
  return true;
}

bool Game::IsLockableBy(const UnitLocation& current, Command cmd) const {
  UnitLocation new_unit = Game::NextUnit(current, cmd);
  return board_.IsConflicting(new_unit);
}

Game::Command Game::GetLockCommand(const UnitLocation& current) const {
  for (Command c = Command::E; c != Command::IGNORED; ++c) {
    if (IsLockableBy(current, c))
      return c;
  }
  return Command::IGNORED;
}

bool Game::IsLockable(const UnitLocation& current) const {
  return GetLockCommand(current) != Command::IGNORED;
}

#if 0
struct UnitLocation {
  HexPoint pivot;
  int angle;
  UnitLocation() {}
  UnitLocation(const HexPoint& pivot, int angle) : pivot(pivot), angle(angle) {}
  bool operator<(const UnitLocation& other) const {
    return pivot.x() != other.pivot.x() ? pivot.x() < other.pivot.x() :
      pivot.y() != other.pivot.y() ? pivot.y() < other.pivot.y() :
      angle < other.angle;
  }
  UnitLocation(const Unit& unit)
    : pivot(unit.pivot()), angle(unit.angle()) {}
};
#endif

void Game::ReachableUnits(std::vector<SearchResult>* result) const {
  result->clear();
  {
    Command c = GetLockCommand(current_unit_);
    if (c != Command::IGNORED) {
      result->emplace_back(SearchResult(current_unit_, {c}));
    }
  }

  // TODO: Don't copy vector<command> too much. Use dfs instead?
  std::queue<SearchResult> todo;
  todo.push(SearchResult(current_unit_, {}));
#define USE_BIT_MAP 1
#if USE_BIT_MAP
  Bound bound;
  for (int i = 0; i < data_->units().size(); ++i) {
    if (current_unit_.unit() == &data_->units()[i]) {
      bound = data_->unit_pivot_bounds()[i];
    }
  }

  int pivot_top = bound.top;
  int pivot_left = bound.left;
  int pivot_width = bound.right - bound.left + 1;
  int pivot_height = bound.bottom - bound.top + 1;
  int unit_order = current_unit_.unit()->order();
  int pivot_size = pivot_width * pivot_height * unit_order;
  std::vector<bool> covered(pivot_size, false);
  {
    int x = current_unit_.pivot().x() - pivot_left;
    int y = current_unit_.pivot().y() - pivot_top;
    int index = (y * pivot_width + x) * unit_order + current_unit_.angle();
    covered[index] = true;
  }
#else
  std::set<UnitLocation, UnitLocationLess> covered;
  covered.insert(current_unit_);
#endif
  while (!todo.empty()) {
    UnitLocation current = todo.front().first;
    std::vector<Command> moves = std::move(todo.front().second);
    todo.pop();
    for (Command c = Command::E; c != Command::IGNORED; ++c) {
      UnitLocation next = Game::NextUnit(current, c);
      if (board_.IsConflicting(next)) {
        continue;
      }
      // TODO: performance improvement using set and such.
#if USE_BIT_MAP
      int x = next.pivot().x() - pivot_left;
      int y = next.pivot().y() - pivot_top;
      int index = (y * pivot_width + x) * unit_order + next.angle();
      if (covered[index]) {
        continue;
      }
#else
      if (covered.count(UnitLocation(next))) {
        continue;
      }
#endif
      moves.emplace_back(c);
      todo.push(SearchResult(next, moves));
#if USE_BIT_MAP
      covered[index] = true;
#else
      covered.insert(UnitLocation(next));
#endif
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
  *os << "current_index: " << current_index_ << "\n";
  *os << "Rand: " << rand_.current() << "\n";
  *os << "Score: " << score_ << "\n";

  *os << "Map:";
  {
    for (size_t y = 0; y < board_.height(); ++y) {
      *os << "\n";
      if (y & 1) {
        *os << ' ';
      }

      for (size_t x = 0; x < board_.width(); ++x) {
        if (x > 0) {
          *os << ' ';
        }

        char c = '.';
        if (board_(x, y)) {
          c = '*';
        } else {
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
        }
        *os << c;
      }
    }
  }
}

