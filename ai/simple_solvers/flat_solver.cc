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

template<typename T>
std::string DumpV(const std::vector<T>& seq) {
  std::ostringstream ofs;
  for (const auto& e : seq) {
    ofs << e << ", ";
  }
  return ofs.str();
}

class FlatSolver : public Solver {
public:
  FlatSolver() {}
  virtual ~FlatSolver() {}

  int Score(const Game& game) const {
    const Board& board(game.board());
    std::vector<int> height(board.width(), -1);
    std::vector<int> hole(board.width());
    for (int i = 0; i < board.width(); ++i) {
      for (int j = 0; j < board.height(); ++j) {
        if (board.cells()[j][i]) {
          if (height[i] < 0) {
            height[i] = j;
          }
        } else {
          if (height[i] >= 0) {
            hole[i] ++;
          }
        }
      }
      if (height[i] < 0) {
        height[i] = board.height();
      }
    }

    int height_score = 0;
    for (int i = 0; i < board.width() - 1; ++i) {
      const auto& diff = height[i + 1] - height[i];
      height_score += diff * diff;
    }
    int hole_score = 0;
    for (int i = 0; i < board.width(); ++i) {
      hole_score += hole[i];
    }
    return game.score() - height_score * 100 - hole_score * 2000;
  }

  virtual std::string NextCommands(const Game& game) {
    std::vector<Game::Command> ret;

    std::vector<Game::SearchResult> bfsresult;
    game.ReachableUnits(&bfsresult);
    int max_score = std::numeric_limits<int>::min();
    VLOG(1) << "next hands:" << bfsresult.size();
    for(const auto &res: bfsresult) {
      Game ng(game);
      int score = 0;
      if (ng.RunSequence(res.second)) {
        score = Score(ng);
      } else {
        score = ng.score() - 100 * 10000;
      }
      if (score > max_score) {
        VLOG(1) << "@" << res.first.pivot() << "-" << res.first.angle()
                << " score:" << max_score << " -> " << score;
        ret = res.second;
        max_score = score;
      }
    }
    return Game::Commands2SimpleString(ret);
  }
};

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logtostderr = true;
  return RunSolver(new FlatSolver(), "Flat");
}
