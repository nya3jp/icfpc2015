#include <algorithm>

#include "board.h"

Board::Board() : width_(0), height_(0) {
}

Board::~Board() {
}

void Board::Load(const picojson::value& parsed) {
  width_ = parsed.get("width").get<int64_t>();
  height_ = parsed.get("height").get<int64_t>();
  cells_ = Map(width_ * height_, false);

  // Fill the board.
  const picojson::array& filled = parsed.get("filled").get<picojson::array>();
  for (const auto& value : filled) {
    int x = value.get("x").get<int64_t>();
    int y = value.get("y").get<int64_t>();
    cells_[y * width_ + x] = true;
  }
}

bool Board::IsConflicting(const UnitLocation& unit) const {
  for (const auto& member : unit.members()) {
    // Hack!!! The following operation is equivalent to
    // (y < 0 || height_ <= y) || (x < 0 || width_ <= x)
    if (height_ <= static_cast<unsigned int>(member.y()) ||
        width_ <= static_cast<unsigned int>(member.x())) {
      return true;
    }
    if (cells_[member.y() * width_ + member.x()]) {
      return true;
    }
  }
  return false;
}

int Board::Lock(const UnitLocation& unit) {
  for (const auto& member: unit.members()) {
    cells_[member.y() * width_ + member.x()] = true;
  }

  // Clears for each row if necessary.
  int num_cleared_lines = 0;
  for (int y = height_ - 1; y >= 0; --y) {
    size_t begin = y * width_;
    size_t end = begin + width_;

    if (std::all_of(cells_.begin() + begin, cells_.begin() + end,
                    [](bool v) { return v; })) {
      ++num_cleared_lines;
    } else {
      // Fall.
      size_t dest_begin = (y + num_cleared_lines) * width_;
      std::copy(cells_.begin() + begin, cells_.begin() + end,
                cells_.begin() + dest_begin);
    }
  }

  // Clear top lines.
  std::fill(cells_.begin(),
            cells_.begin() + num_cleared_lines * width_,
            false);
  return num_cleared_lines;
}

int Board::LockPreview(const UnitLocation& unit) const {
  int num_cleared_lines = 0;
  for (int y = height_ - 1; y >= 0; --y) {
    bool ok = true;
    for (size_t x = 0; x < width_; ++x) {
      if (cells_[y * width_ + x])
        continue;

      bool hit = false;
      for (const auto& member: unit.members()) {
        if (y == member.y() && x == member.x()) {
          hit = true;
          break;
        }
      }
      if (hit)
        continue;

      ok = false;
      break;
    }
    if (ok) {
      ++num_cleared_lines;
    }
  }
  return num_cleared_lines;
}

void Board::Dump(std::ostream* os) const {
  for (size_t y = 0; y < height_; ++y) {
    if (y & 1) {
      *os << ' ';
    }
    for (size_t x = 0; x < width_; ++x) {
      *os << (cells_[y * width_ + x] ? '*' : '.');
      if (x + 1 < width_) {
        *os << ' ';
      }
    }
    if (y + 1 < height_) {
      *os << '\n';
    }
  }
}

