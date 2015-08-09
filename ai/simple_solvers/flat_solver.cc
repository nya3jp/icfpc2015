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
    VLOG(1) << "map_height:" << DumpV(map_height);

    std::vector<Game::SearchResult> bfsresult;
    game.ReachableUnits(&bfsresult);
    int min_score = std::numeric_limits<int>::max();
    for(const auto &res: bfsresult) {
      const Unit &u = res.first;
      std::vector<int> height(map_height);
      std::vector<int> hole(map_hole);
      for(const auto &m: u.members()) {
        if (height[m.x()] < m.y()) {  // Hole filled.
          --hole[m.x()];
        } else {
          // Maybe a new hole.
          hole[m.x()] += height[m.x()] - m.y() - 1;
          height[m.x()] = m.y();
        }
      }
      int height_score = 0;
      for (int i = 0; i < board.width() - 1; ++i) {
        const auto& diff = height[i + 1] - height[i];
        height_score += diff * diff;
      }
      int hole_score = 0;
      for (int i = 0; i < board.width(); ++i) {
        hole_score += hole[i] * hole[i];
      }
      int score = height_score + hole_score * 20;
      if (score < min_score) {
        VLOG(1) << "@" << u.pivot() << "-" << u.angle()
                  << " :" << DumpV(height)
                  << " :" << DumpV(hole)
                  << " score:" << min_score << " -> " << score
                  << "(" << height_score << "," << hole_score << ")";
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
  return RunSolver(new FlatSolver(), "Flat");
}
