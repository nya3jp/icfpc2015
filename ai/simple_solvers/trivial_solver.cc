#include <algorithm>
#include <limits>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <glog/logging.h>

#include "../../simulator/board.h"
#include "../../simulator/game.h"
#include "../../simulator/solver.h"
#include "../../simulator/unit.h"

namespace {
int GetTop(const UnitLocation& unit) {
  int top = std::numeric_limits<int>::max();
  for (const auto& member : unit.members()) {
    top = std::min(top, member.y());
  }
  return top;
}

int GetBottom(const UnitLocation& unit) {
  int bottom = -1;
  for (const auto& member : unit.members()) {
    bottom = std::max(bottom, member.y());
  }
  return bottom;
}

int GetLeft(const UnitLocation& unit) {
  int left = std::numeric_limits<int>::max();
  for (const auto& member : unit.members()) {
    left = std::min(left, member.x());
  }
  return left;
}

int GetRight(const UnitLocation& unit) {
  int right = -1;
  for (const auto& member : unit.members()) {
    right = std::max(right, member.x());
  }
  return right;
}

}

class TrivialSolver : public Solver {
public:
  TrivialSolver() {}
  virtual ~TrivialSolver() {}

  HexPoint GetTetrisPosition(const Board& board, int y) {
    HexPoint tetris_point(board.width() - 1, board.height() - 1);
    for (int i = 0; i < board.height() - 1 - y; ++i)
      tetris_point.MoveNorthWest();
    return tetris_point;
  }

  static int GetEmptyLocationID(const Board& board,
                                const std::vector<HexPoint>& members,
                                const HexPoint& p) {
    if (p.x() < 0 || p.x() >= board.width() || p.y() < 0 || p.y() >= board.height())
      return -1;
    if (board(p.x(), p.y()))
      return -1;

    for (const auto& member : members) {
      if (p.x() == member.x() && p.y() == member.y())
        return -1;
    }

    return p.x() + p.y() * board.width();
  }

  int CountSections(const Board& board, const std::vector<HexPoint>& members) {
    std::queue<int> todo;
    std::set<int> covered;

    int result = 0;

    while (true) {
      bool found = false;
      int x = 0;
      int y = 0;
      for (; x < board.width(); ++x) {
        for (y = 0; y < board.height(); ++y) {
          if (board(x, y))
            continue;

          if (covered.count(y * board.width() + x))
            continue;

          bool hit = false;
          for (const auto& member : members) {
            if (x == member.x() && y == member.y()) {
              hit = true;
              break;
            }
          }
          if (!hit) {
            found = true;
            break;
          }
        }
        if (found)
          break;
      }

      if (!found)
        break;

      ++result;
      int id = y * board.width() + x;
      todo.push(id);
      covered.insert(id);
      while (!todo.empty()) {
        int current = todo.front();
        todo.pop();

        int xx = current % board.width();
        int yy = current / board.width();

        {
          HexPoint p(xx, yy);
          p.MoveEast();
          int movedId = GetEmptyLocationID(board, members, p);
          if (movedId != -1 && covered.count(movedId) == 0) {
            todo.push(movedId);
            covered.insert(movedId);
          }
        }

        {
          HexPoint p(xx, yy);
          p.MoveWest();
          int movedId = GetEmptyLocationID(board, members, p);
          if (movedId != -1 && covered.count(movedId) == 0) {
            todo.push(movedId);
            covered.insert(movedId);
          }
        }

        {
          HexPoint p(xx, yy);
          p.MoveNorthEast();
          int movedId = GetEmptyLocationID(board, members, p);
          if (movedId != -1 && covered.count(movedId) == 0) {
            todo.push(movedId);
            covered.insert(movedId);
          }
        }

        {
          HexPoint p(xx, yy);
          p.MoveNorthWest();
          int movedId = GetEmptyLocationID(board, members, p);
          if (movedId != -1 && covered.count(movedId) == 0) {
            todo.push(movedId);
            covered.insert(movedId);
          }
        }

        {
          HexPoint p(xx, yy);
          p.MoveSouthEast();
          int movedId = GetEmptyLocationID(board, members, p);
          if (movedId != -1 && covered.count(movedId) == 0) {
            todo.push(movedId);
            covered.insert(movedId);
          }
        }

        {
          HexPoint p(xx, yy);
          p.MoveSouthWest();
          int movedId = GetEmptyLocationID(board, members, p);
          if (movedId != -1 && covered.count(movedId) == 0) {
            todo.push(movedId);
            covered.insert(movedId);
          }
        }
      }
    }

    return result;
  }

  std::string Tetris(const Game& game,
                     const std::vector<Game::SearchResult>& bfsresult) {
    std::vector<Game::Command> ret;

    int max_cleared = -1;
    int candidate_top = std::numeric_limits<int>::max();
    for (const auto &res: bfsresult) {
      int cleared = game.GetBoard().LockPreview(res.first);
      if (cleared > max_cleared ||
          (cleared == max_cleared && GetTop(res.first) < candidate_top)) {
        max_cleared = cleared;
        candidate_top = GetTop(res.first);
        ret = res.second;
      }
    }

    if (max_cleared <= 0) {
      return SouthWest(game, bfsresult);
    }

    return Game::Commands2SimpleString(ret);
  }

  virtual std::string NextCommands(const Game& game) {
    std::vector<Game::SearchResult> bfsresult;
    game.ReachableUnits(&bfsresult);

    if (GetBottom(game.current_unit()) != GetTop(game.current_unit())) {
      return Tetris(game, bfsresult);
      //return SouthWest(game, bfsresult);
    }

    const Board& board = game.GetBoard();
    for (size_t yy = 0; yy < board.height(); ++yy) {
      size_t y = board.height() - 1 - yy;

      const HexPoint tetris_point = GetTetrisPosition(board, y);

      for (size_t x = 0; x < board.width(); ++x) {
        if (y <= 1 && x > 3) {
          return Tetris(game, bfsresult);
        }

        if (x == tetris_point.x())
          continue;

        if (board(x, y))
          continue;

        for (const auto &res: bfsresult) {
          int top = GetTop(res.first);
          int right = GetRight(res.first);
          int bottom = GetBottom(res.first);
          int left = GetLeft(res.first);

          if (bottom == y && top == y) {
            if (left == x &&
                (tetris_point.x() > right ||
                 tetris_point.x() < left)) {
              return Game::Commands2SimpleString(res.second);
            }
          }
        }
      }
    }

    return SouthWest(game, bfsresult);
  }

  std::string SouthWest(const Game& game,
                        const std::vector<Game::SearchResult>& bfsresult) {
    std::vector<Game::Command> ret;
    ret.push_back(Game::Command::SW);

    int candidate_bottom = 0;
    int candidate_left = std::numeric_limits<int>::max();
    int candidate_increase = std::numeric_limits<int>::max();

    for (const auto &res: bfsresult) {
      int bottom = GetBottom(res.first);
      int left = GetLeft(res.first);

      // std::vector<HexPoint> empty;
      // int before = CountSections(game.GetBoard(), empty);
      // int after = CountSections(game.GetBoard(), res.first.members());
      // int increase = after - before;
      int increase = std::numeric_limits<int>::max();

      if (increase > candidate_increase)
        continue;

      if (increase == candidate_increase) {
        if (bottom < candidate_bottom)
          continue;

        if (bottom == candidate_bottom) {
          if (left >= candidate_left)
            continue;
        }
      }

      candidate_bottom = bottom;
      candidate_left = left;
      candidate_increase = increase;

      ret = res.second;
    }

    return Game::Commands2SimpleString(ret);
  }
};

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logtostderr = true;
  return RunSolver(new TrivialSolver(), "Trivial");
}
