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
  current_index_ = -1;
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
  *os << "Rand: " << rand_.seed() << ", " << rand_.current();
}

std::ostream& operator<<(std::ostream& os, const Game& game) {
  game.Dump(&os);
  return os;
}
