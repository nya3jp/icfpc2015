#include <algorithm>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <glog/logging.h>

#include "../../simulator/board.h"
#include "../../simulator/game.h"
#include "../../simulator/solver.h"
#include "../../simulator/unit.h"

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

  std::string Tetris(const Game& game,
                     const std::vector<Game::SearchResult>& bfsresult) {
    std::vector<Game::Command> ret;

    int max_cleared = -1;
    int candidate_top = std::numeric_limits<int>::max();
    for (const auto &res: bfsresult) {
      int cleared = game.GetBoard().LockPreview(res.first);
      if (cleared > max_cleared ||
          (cleared == max_cleared && res.first.GetTop() < candidate_top)) {
        max_cleared = cleared;
        candidate_top = res.first.GetTop();
        ret = res.second;
      }
    }

    return Game::Commands2SimpleString(ret);
  }

  virtual std::string NextCommands(const Game& game) {
    std::vector<Game::SearchResult> bfsresult;
    game.ReachableUnits(&bfsresult);

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
          int top = res.first.GetTop();
          int right = res.first.GetRight();
          int bottom = res.first.GetBottom();
          int left = res.first.GetLeft();

          if (bottom == y && left == x && top == y &&
              (tetris_point.x() > right ||
               tetris_point.x() < left)) {
            return Game::Commands2SimpleString(res.second);
          }
        }
      }
    }

    std::vector<Game::Command> ret;
    ret.push_back(Game::Command::SW);
    return Game::Commands2SimpleString(ret);
  }
};

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logtostderr = true;
  return RunSolver(new TrivialSolver(), "Trivial");
}
