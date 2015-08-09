#include <algorithm>
#include <string>
#include <vector>

#include "board.h"
#include "game.h"
#include "solver.h"
#include "unit.h"


class FlatSolver : public Solver {
public:
  FlatSolver() {}
  virtual ~FlatSolver() {}

  virtual std::string NextCommands(const Game& game) {
    std::vector<Game::Command> ret;

    const Board& board(game.board());
    std::vector<int> map_height(board.width(), -1);
    std::vector<int> map_hole(board.width());
    for (int i = 0; i < board.width(); ++i) {
      for (int j = 0; j < board.height(); ++j) {
        if (board.cells()[j][i]) {
          if (map_height[i] < 0) {
            map_height[i] = j;
          }
        } else {
          if (map_height[i] >= 0) {
            map_hole[i] ++;
          }
        }
      }
      if (map_height[i] < 0) {
        map_height[i] = board.height();
      }
    }

    std::vector<Game::SearchResult> bfsresult;
    game.ReachableUnits(&bfsresult);
    int min_score = board.height() * board.height() * board.width();
    for(const auto &res: bfsresult) {
      const Unit &u = res.first;
      std::vector<int> height(map_height);
      std::vector<int> hole(map_hole);
      for(const auto &m: u.members()) {
        if (height[m.x()] < m.y()) {
          --hole[m.x()];
        } else {
          hole[m.x()] += height[m.x()] - m.y() - 1;
        }
      }
      int score = 0;
      for (int i = 0; i < board.width() - 1; ++i) {
        score += (height[i] - height[i + 1]) * (height[i] - height[i + 1]);
      }
      for (int i = 0; i < board.width(); ++i) {
        score += hole[i] * hole[i];
      }
      if (score < min_score) {
          ret = res.second;
          min_score = score;
      }
    }
    return Game::Commands2SimpleString(ret);
  }
};

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logtostderr = true;
  return RunSolver(new FlatSolver(), "FlatSolver");
}
