#include "board.h"

Board::Board() {
}

Board::~Board() {
}

void Board::Load(const picojson::value& parsed) {
  int width = parsed.get("width").get<int64_t>();
  int height = parsed.get("height").get<int64_t>();
  Init(width, height);

  // Fill the board.
  const picojson::array& filled = parsed.get("filled").get<picojson::array>();
  for (const auto& value : filled) {
    int x = value.get("x").get<int64_t>();
    int y = value.get("y").get<int64_t>();
    cells_[y][x] = 1;
  }
}

void Board::Init(int width, int height) {
  cells_ = Map(height, std::vector<int>(width));
}

void Board::Dump(std::ostream* os) const {
  for (size_t y = 0; y < cells_.size(); ++y) {
    if (y & 1) {
      *os << ' ';
    }
    const std::vector<int>& row = cells_[y];
    for (size_t x = 0; x < row.size(); ++x) {
      *os << (row[x] == 0 ? '.' : '*');
      if (x + 1 < row.size()) {
        *os << ' ';
      }
    }
    if (y + 1 < cells_.size()) {
      *os << '\n';
    }
  }
}

std::ostream& operator<<(std::ostream& os, const Board& board) {
  board.Dump(&os);
  return os;
}
