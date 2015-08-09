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
