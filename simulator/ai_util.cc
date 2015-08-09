#include "ai_util.h"

#include <vector>
#include <stack>
#include <utility>

#include "game.h"

std::vector<int> GetHeightLine(const Game& game) {
  const Board& board(game.board());
  std::vector<int> height(board.width(), board.height());
  for (int i = 0; i < board.width(); ++i) {
    for (int j = 0; j < board.height(); ++j) {
      if (board(i, j)) {
        height[i] = j;
        break;
      }
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

  int movepattern[] = { -1, 0,
                        1, 0,
                        -1, 1,
                        0, 1};

  std::vector<bool> visited(width * height, false);
  const Board& board = game.GetBoard();

  std::stack<std::pair<int, int> > dfs;
  
  for(int i = 0; i < width; ++i) {
    if(!board(i, 0)) {
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
      if(movepattern[i + 1] == 1) {
        newx += (y & 1);
      }
      
      if((newx < 0) || (newx >= width) || 
         (newy < 0) || (newy >= height)) {
        continue;
      }
      if(board(newx, newy) || visited[newy * width + newx]) {
        continue;
      }
      dfs.push(std::make_pair(newx, newy));
    }
  }

  return ret;
}


void GetReachabilityMapByAnyHands(const Game& game, Board *ret_board)
{
  *ret_board = game.GetBoard();
  for(int h = 0; h < *ret_board.height(); ++h) {
    for(int w = 0; w < *ret_board.width(); ++w) {
      *ret_board.Set(w, h, false);
    }
  }

  for(size_t index = 0; index < game.GetUnits().size(); ++index) {
    const Unit &u = game.GetUnits()[index];
    std::stack<UnitLocation> todo;
    todo.push(SearchResult(u, {}));
    std::set<UnitLocation> covered;
    while (!todo.empty()) {
      Unit current = todo.top().first;
      todo.pop();
      covered.insert(UnitLocation(todo));
      for(const auto& p: current.members()) {
        *ret_borad.Set(p, true);
      }
      for (Command c = Command::E; c != Command::IGNORED; ++c) {
        Unit next = Game::NextUnit(current, c);
        if (covered.count(UnitLocation(next))) {
          continue;
        }
        if (board_.IsConflicting(next)) {
          continue;
        }
        todo.push(next);
        covered.insert(UnitLocation(next));
      }
    }
  }
}
