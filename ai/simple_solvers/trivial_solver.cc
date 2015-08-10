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

  std::set<int> GetTetrisPositions(const Board& board, const HexPoint& target) {
    std::set<int> result;
    result.insert(target.x() + target.y() * board.width());
    HexPoint p(target.x(), target.y());
    for (int i = 0; i < target.y(); ++i) {
      p.MoveNorthWest();
      if (p.x() < 0 || p.x() >= board.width() || p.y() < 0 || p.y() >= board.height())
        break;
      result.insert(p.x() + p.y() * board.width());
    }
    return result;
  }

  static int GetEmptyLocationID(const Board& board,
                                const UnitLocation::Members& members,
                                const HexPoint& p,
                                std::set<int> covered,
                                std::queue<int> todo,
                                int& score) {
    if (p.x() < 0 || p.x() >= board.width() || p.y() < 0 || p.y() >= board.height()) {
      //++score;
      return -1;
    }
    if (board(p.x(), p.y())) {
      score += 2;
      return -1;
    }

    for (const auto& member : members) {
      if (p.x() == member.x() && p.y() == member.y()) {
        score += 2;
        return -1;
      }
    }

    int id = p.x() + p.y() * board.width();

    if (covered.count(id) == 0) {
      todo.push(id);
      covered.insert(id);
    }
  }

  int CountSections(const Board& board, const UnitLocation::Members& members) {
    std::queue<int> todo;
    std::set<int> covered;

    int score = 0;

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

      int id = y * board.width() + x;
      todo.push(id);
      covered.insert(id);
      while (!todo.empty()) {
        int current = todo.front();
        todo.pop();

        int xx = current % board.width();
        int yy = current / board.width();

        int count = 0;

        {
          HexPoint p(xx, yy);
          p.MoveEast();
          GetEmptyLocationID(board, members, p, covered, todo, count);
        }

        {
          HexPoint p(xx, yy);
          p.MoveWest();
          GetEmptyLocationID(board, members, p, covered, todo, count);
        }

        {
          HexPoint p(xx, yy);
          p.MoveNorthEast();
          GetEmptyLocationID(board, members, p, covered, todo, count);
        }

        {
          HexPoint p(xx, yy);
          p.MoveNorthWest();
          GetEmptyLocationID(board, members, p, covered, todo, count);
        }

        {
          HexPoint p(xx, yy);
          p.MoveSouthEast();
          GetEmptyLocationID(board, members, p, covered, todo, count);
        }

        {
          HexPoint p(xx, yy);
          p.MoveSouthWest();
          GetEmptyLocationID(board, members, p, covered, todo, count);
        }

        score += count * count;
      }
    }

    return -score;
  }

  std::string Tetris(const Game& game,
                     const std::vector<Game::SearchResult>& bfsresult) {
    std::vector<Game::Command> ret;

    int max_cleared = -1;
    int highest_top = std::numeric_limits<int>::max();
    for (const auto &res: bfsresult) {
      int cleared = game.GetBoard().LockPreview(res.first);
      int top = GetTop(res.first);

      if (cleared < max_cleared)
        continue;

      if (cleared == max_cleared) {
        if (top >= highest_top)
          continue;
      }

      max_cleared = cleared;
      highest_top = top;
      ret = res.second;
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
    }

    const Board& board = game.GetBoard();

    int highest_density = 0;
    int most_dense_y = -1;
    int most_dense_right_most_empty_x = -1;

    for (int y = board.height() - 1; y >= 0; --y) {
      int density = 0;
      int right_most_empty_x = -1;
      for (int x = board.width() - 1; x >= 0; --x) {
        if (board(x, y)) {
          ++density;
        } else {
          if (right_most_empty_x == -1)
            right_most_empty_x = x;
        }
      }

      if (density > highest_density) {
        highest_density = density;
        most_dense_y = y;
        most_dense_right_most_empty_x = right_most_empty_x;
      }
    }

    for (const auto &res: bfsresult) {
      int cleared = game.GetBoard().LockPreview(res.first);
      if (cleared > 5)
        return Tetris(game, bfsresult);
    }

    if (game.prev_cleared_lines() > 1) {
      return Tetris(game, bfsresult);
    }

    const HexPoint target_position(most_dense_right_most_empty_x, most_dense_y);
    const std::set<int> tetris_positions = GetTetrisPositions(board, target_position);

    for (int y = most_dense_y; y >= 0; --y) {
      for (int x = 0; x < board.width(); ++x) {
        if (y <= 1 && x + game.current_unit().members().size() >= 5) {
          DVLOG(1) << "TETRIS MODE";
          return Tetris(game, bfsresult);
        }

        if (board(x, y))
          continue;

        std::vector<Game::Command> ret;

        int lowest_top = std::numeric_limits<int>::min();
        int most_left = std::numeric_limits<int>::max();

        bool found = false;

        for (const auto &res: bfsresult) {
          bool conflict = false;
          bool hit = false;
          for (const auto& member : res.first.members()) {
            if (tetris_positions.count(member.x() + member.y() * board.width())) {
              conflict = true;
              break;
            }

            if (member.x() == x && member.y() == y)
              hit = true;
          }

          if (conflict)
            continue;

          if (!hit)
            continue;

          int top = GetTop(res.first);
          int bottom = GetBottom(res.first);
          int left = GetLeft(res.first);

          if (top != bottom)
            continue;

          if (top < lowest_top)
            continue;

          if (top == lowest_top) {
            if (left > most_left)
              continue;
          }

          found = true;

          lowest_top = top;
          most_left = left;
          ret = res.second;
        }

        if (!found)
          continue;

        DVLOG(1) << "MAKE TETRISABLE " << x << ", " << y;

        return Game::Commands2SimpleString(ret);
      }
    }

    return SouthWest(game, bfsresult);
  }

  std::string SouthWest(const Game& game,
                        const std::vector<Game::SearchResult>& bfsresult) {
    std::vector<Game::Command> ret;
    ret.push_back(Game::Command::SW);

    int candidate_top = std::numeric_limits<int>::min();
    int candidate_right = std::numeric_limits<int>::max();

    for (const auto &res: bfsresult) {
      int top = GetTop(res.first);
      int right = GetRight(res.first);

      if (top < candidate_top)
        continue;

      if (top == candidate_top) {
        if (right > candidate_right)
          continue;

        if (right == candidate_right) {
          continue;
        }
      }

      candidate_top = top;
      candidate_right = right;

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
