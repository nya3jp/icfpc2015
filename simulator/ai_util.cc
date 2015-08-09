#include "ai_util.h"

#include <vector>
#include <stack>
#include <utility>

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

int64_t GetDotReachabilityFromTop(const Game& game)
{
  int64_t ret = 0;
  int width = game.GetBoard().width();
  int height = game.GetBoard().height();

  int movepattern[] = { 0, -1, 
                        1, 0,
                        0, 1,
                        -1, 0 };

  std::vector<bool> visited(width * height, false);
  const Board::Map& cells = game.GetBoard().cells();

  std::stack<std::pair<int, int> > dfs;
  
  for(int i = 0; i < width; ++i) {
    if(!cells[0][i]) {
      dfs.push(std::make_pair(i, 0));
    }
  }

  while(!dfs.empty()) {
    int x = dfs.top().first;
    int y = dfs.top().second;
    dfs.pop();
    if(visited[y * width + x]) continue;
    ret++;
    visited[y * width + x] = true;
    
    for(int i = 0; i < 8; i += 2) {
      int newx = x + movepattern[i];
      int newy = y + movepattern[i + 1];
      
      if((newx < 0) || (newx >= width) || 
         (newy < 0) || (newy >= height)) {
        continue;
      }
      if(cells[newy][newx] || visited[newy * width + newx]) {
        continue;
      }
      dfs.push(std::make_pair(newx, newy));
    }
  }

  return ret;
}
