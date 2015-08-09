#include "ai_util.h"

#include <vector>

#include "game.h"

std::vector<int> GetHeightLine(const Game& game) {
  const Board& board(game.board());
  std::vector<int> height(board.width(), -1);
  for (int i = 0; i < board.width(); ++i) {
    for (int j = 0; j < board.height(); ++j) {
      if (board.cells()[j][i]) {
        if (height[i] < 0) {
          height[i] = j;
        }
      }
    }
    if (height[i] < 0) {
      height[i] = board.height();
    }
  }
  return height;
}

int64_t GetHeightPenalty(const std::vector<int>& height) {
  int64_t height_score = 0;
  for (int i = 0; i < height.size() - 1; ++i) {
    const auto& diff = height[i + 1] - height[i];
    height_score += diff * diff;
  }
  return height_score;
}

int64_t GetHeightPenaltyFromGame(const Game& game) {
  return GetHeightPenalty(GetHeightLine(game));
}
