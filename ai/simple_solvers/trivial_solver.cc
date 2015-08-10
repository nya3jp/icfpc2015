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
#include "../../simulator/ai_util.h"

namespace {

  void GetBoundBox(const UnitLocation& unit,
                  int* top, int* right, int* bottom, int* left) {
    *top = std::numeric_limits<int>::max();
    *bottom = -1;
    *left = std::numeric_limits<int>::max();
    *right = -1;
    for (const auto& member : unit.members()) {
      *top = std::min(*top, member.y());
      *bottom = std::max(*bottom, member.y());
      *left = std::min(*left, member.x());
      *right = std::max(*right, member.x());
    }
  }

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
  TrivialSolver()
    : checked_units_(false),
      is_bar_only_game_(false),
      max_unit_size_(0) {}
  virtual ~TrivialSolver() {}

  bool IsForbidden(const Board& board, const HexPoint& p) {
    return (p.x() <= board.width() / 2 + 2) &&
      (p.x() >= board.width() / 2 - 2) &&
      (p.y() <= 2);
  }

  static bool InBoard(const Board& board, const HexPoint& p) {
    return
      p.x() >= 0 && p.x() < board.width() &&
                            p.y() >= 0 && p.y() < board.height();
  }

  std::set<int> GetTetrisPositions(const Board& board, const HexPoint& target) {
    std::set<int> result;
    result.insert(target.x() + target.y() * board.width());
    HexPoint p(target.x(), target.y());
    for (int i = 0; i < target.y(); ++i) {
      p.MoveNorthWest();
      if (!InBoard(board, p))
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
    if (!InBoard(board, p)) {
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

      //++score;

      int id = y * board.width() + x;
      todo.push(id);
      covered.insert(id);
      while (!todo.empty()) {
        int current = todo.front();
        todo.pop();

        int xx = current % board.width();
        int yy = current / board.width();

        int count = 0;

        CheckNeighbor(xx, yy, board, members, covered, todo, count);

        //score += count * count;
      }
    }

    return -score;
  }

  int CountContact(const Board& board, const UnitLocation::Members& members) {
    int score = 0;

    for (const auto& member : members) {
      int x = member.x();
      int y = member.y();

      std::vector<HexPoint> moves;

      {
        HexPoint p(x, y);
        p.MoveEast();
        moves.push_back(p);
      }
      {
        HexPoint p(x, y);
        p.MoveWest();
        moves.push_back(p);
      }
      {
        HexPoint p(x, y);
        p.MoveNorthEast();
        moves.push_back(p);
      }
      {
        HexPoint p(x, y);
        p.MoveNorthWest();
        moves.push_back(p);
      }
      {
        HexPoint p(x, y);
        p.MoveSouthEast();
        moves.push_back(p);
      }
      {
        HexPoint p(x, y);
        p.MoveSouthWest();
        moves.push_back(p);
      }

      for (const auto& move : moves) {
        if (InBoard(board, move)) {
          if (board(move.x(), move.y()))
            score += 2;
        } else {
          score += 1;
        }
      }
    }

    return score;
  }

  void CheckNeighbor(int xx, int yy,
                     const Board& board,
                     const UnitLocation::Members& members,
                     std::set<int> covered,
                     std::queue<int> todo,
                     int& count) {
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
  }

  std::string Tetris(const Game& game,
                     const std::vector<Game::SearchResult>& bfsresult,
                     const std::set<int> tetris_positions) {
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
      return SouthWest(game, bfsresult, tetris_positions);
    }

    return Game::Commands2SimpleString(ret);
  }

  bool checked_units_;
  bool is_bar_only_game_;
  int max_unit_size_;

  bool IsBar(const Unit& unit) {
    const HexPoint origin(0, 0);
    UnitLocation loc(&unit, origin);

    int top, right, bottom, left;

    GetBoundBox(loc, &top, &right, &bottom, &left);
    if (top == bottom)
      return true;

    loc.RotateClockwise();
    GetBoundBox(loc, &top, &right, &bottom, &left);
    if (top == bottom)
      return true;

    loc.RotateClockwise();
    GetBoundBox(loc, &top, &right, &bottom, &left);
    if (top == bottom)
      return true;

    return false;
  }

  virtual std::string NextCommands(const Game& game) {
    if (!checked_units_) {
      checked_units_ = true;

      bool non_bar_found = false;
      for (const auto& unit : game.units()) {
        max_unit_size_ = std::max(max_unit_size_,
                                  static_cast<int>(unit.members().size()));

        if (!IsBar(unit)) {
          non_bar_found = true;
        }
      }

      if (non_bar_found) {
        is_bar_only_game_ = false;
      } else {
        is_bar_only_game_ = true;
      }
    }

    if (!is_bar_only_game_) {
      if (true) {
        std::vector<Game::Command> ret;
        ret.push_back(Game::Command::SW);
        return Game::Commands2SimpleString(ret);
      }
    }

    std::vector<Game::SearchResult> bfsresult;
    game.ReachableUnits(&bfsresult);

    const Board& board = game.GetBoard();

    std::set<int> tetris_positions;
    HexPoint target_position;
    int highest_density = 0;

    //Board rboard(board.width(), board.height());
    //GetDotReachabilityFromTopAsMap(game, &rboard);

    for (int y = 0; y < board.height(); ++y) {
      int density = 0;

      bool bad = false;
      for (int x = board.width() - 1; x >= 0; --x) {
        if (board(x, y)) {
          ++density;
        } else {
          // if (!rboard(x, y)) {
          //   bad = true;
          //   break;
          // }
        }
      }

      if (bad)
        continue;

      for (int x = board.width() - 1; x >= 0; --x) {
        if (!board(x, y)) {
          HexPoint candidate(x, y);
          std::set<int> candidate_positions =
            GetTetrisPositions(board, candidate);
          if (candidate_positions.size() == y + 1 && density > highest_density) {
            highest_density = density;
            target_position = candidate;
            tetris_positions = candidate_positions;
          }
        }
      }
    }

    for (const auto &res: bfsresult) {
      int cleared = game.GetBoard().LockPreview(res.first);
      if (cleared >= 6 ||
          (cleared > 0 && (game.units_remaining() < 3 ||
                           game.prev_cleared_lines() > 1 ||
                           !is_bar_only_game_ ||
                           max_unit_size_ == 1)))
        return Tetris(game, bfsresult, tetris_positions);
    }




        std::vector<Game::Command> ret;
        int lowest_top = std::numeric_limits<int>::min();
        int most_left = std::numeric_limits<int>::max();
        int score_of_best = std::numeric_limits<int>::min();
        bool found = false;


    for (int y = target_position.y(); y >= 0; --y) {
      for (int x = 0; x < board.width(); ++x) {
        if (y <= 1 && x + game.current_unit().members().size() >= 5) {
          //if (y <= 3 && x + game.current_unit().members().size() >= 5) {
          DVLOG(1) << "TETRIS MODE";
          return Tetris(game, bfsresult, tetris_positions);
        }

        if (board(x, y))
          continue;

        for (const auto &res: bfsresult) {
          bool conflict = false;
          bool hit = false;
          for (const auto& member : res.first.members()) {
            int id = member.x() + member.y() * board.width();
            if (tetris_positions.count(id) ||
                IsForbidden(board, member)) {
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

          int score = 0;

          if (is_bar_only_game_) {
            if (game.units_remaining() >= 4 && top != bottom)
              continue;
          }


          score = CountContact(board, res.first.members());
          score *= 1;

          score += y - x;


          if (score < score_of_best)
            continue;

          if (score == score_of_best) {
            if (is_bar_only_game_) {
              if (top < lowest_top)
                continue;

              if (top == lowest_top) {
                if (left > most_left)
                  continue;
              }
            }
          }

          found = true;

          lowest_top = top;
          most_left = left;
          score_of_best = score;

          ret = res.second;
        }

        //if (!found)
        //  continue;
      }


      if (found)
        return Game::Commands2SimpleString(ret);

    }

    return Tetris(game, bfsresult, tetris_positions);
  }

  std::string SouthWest(const Game& game,
                        const std::vector<Game::SearchResult>& bfsresult,
                        const std::set<int> tetris_positions) {
    std::vector<Game::Command> ret;
    ret.push_back(Game::Command::SW);

    int candidate_top = std::numeric_limits<int>::min();
    int candidate_right = std::numeric_limits<int>::max();

    int score_of_best = 0;

    for (const auto &res: bfsresult) {
      int top = GetTop(res.first);
      int right = GetRight(res.first);

      int score = 0;
      for (const auto& member : res.first.members()) {
        int id = member.x() + member.y() * game.GetBoard().width();
        if (tetris_positions.count(id))
          score = -1;
        if (IsForbidden(game.GetBoard(), member))
          score = -2;
      }

      if (score < score_of_best)
        continue;

      if (score == score_of_best) {
        if (top < candidate_top)
          continue;

        if (top == candidate_top) {
          if (right > candidate_right)
            continue;

          if (right == candidate_right) {
            continue;
          }
        }
      }

      candidate_top = top;
      candidate_right = right;

      score_of_best = score;

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
