#include "ai_util.h"

#include <vector>
#include <stack>
#include <utility>
#include <set>

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

int64_t GetDotReachabilityFromTop(const Game& game) {
  Board b;
  return GetDotReachabilityFromTopAsMap(game, &b);
}

int64_t GetDotReachabilityFromTopAsMap(const Game& game, Board *b)
{
  int64_t ret = 0;
  int width = game.GetBoard().width();
  int height = game.GetBoard().height();
  *b = Board(width, height);

  int movepattern[] = { -1, 0,
                        1, 0,
                        -1, 1,
                        0, 1};

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
    if((*b)(x, y)) continue;
    ret++;
    (*b).Set(x, y, true);
    
    for(int i = 0; i < 8; i += 2) {
      int newx = x + movepattern[i];
      int newy = y + movepattern[i + 1];
      newx += movepattern[i + 1] * (y & 1);
      
      if((newx < 0) || (newx >= width) || 
         (newy < 0) || (newy >= height)) {
        continue;
      }
      if(board(newx, newy) || (*b)(newx, newy)) {
        continue;
      }
      dfs.push(std::make_pair(newx, newy));
    }
  }

  return ret;
}

void GetReachabilityMapByAnyHands(const Game& game, Board *ret_board)
{
  *ret_board = game.GetBoard(); // copy
  for(int h = 0; h < ret_board->height(); ++h) {
    for(int w = 0; w < ret_board->width(); ++w) {
      ret_board->Set(w, h, false);
    }
  }

  for(size_t index = 0; index < game.GetNumberOfUnits(); ++index) {
    const UnitLocation &u = game.GetUnitAtSpawnPosition(index);
    for(size_t j = 0; j < index; ++j) {
      if(u.isEquivalent(game.GetUnitAtSpawnPosition(j))) {
        continue;
      }
    }
    std::stack<UnitLocation> todo;
    todo.push(u);
    std::set<UnitLocation, UnitLocationLess> covered;
    while (!todo.empty()) {
      UnitLocation current = todo.top();
      todo.pop();
      covered.insert(UnitLocation(current));
      for(const auto& p: current.members()) {
        ret_board->Set(p, true);
      }
      for (Game::Command c = Game::Command::E; c != Game::Command::IGNORED; ++c) {
        UnitLocation next = Game::NextUnit(current, c);
        if (covered.count(next)) {
          continue;
        }
        if (game.GetBoard().IsConflicting(next)) {
          continue;
        }
        todo.push(next);
        covered.insert(UnitLocation(next));
      }
    }
  }
}
