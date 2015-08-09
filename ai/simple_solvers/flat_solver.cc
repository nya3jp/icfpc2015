#include <algorithm>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <glog/logging.h>

#include "../../simulator/ai_util.h"
#include "../../simulator/board.h"
#include "../../simulator/game.h"
#include "../../simulator/hexpoint.h"
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

  static int64_t Score(const Game& game, std::ostream& os) {
    const Board& board(game.board());
    std::vector<int> height(GetHeightLine(game));
    int hole;
    for (int i = 0; i < board.width(); ++i) {
      for (int j = height[i] + 1; j < board.height(); ++j) {
        if (!board.cells()[j][i]) {
          hole ++;
        }
      }
    }

    int64_t height_score = 0;
    for (int i = 0; i < board.width() - 1; ++i) {
      const auto& diff = height[i + 1] - height[i];
      height_score += diff * diff;
    }
    os << "height:" << DumpV(height) << "(score:" << height_score
       << ") hole:" << hole;
    return game.score() - height_score * 100 - hole * 2000;
  }

  static int64_t MinScore(const Game& game) {
    const Board& board(game.board());
    const int64_t height = board.height();
    const int64_t width = board.width();
    return game.score() -
      2 * (height * width * height * 100 + height * width * 2000);
  }

  virtual std::string NextCommands(const Game& game) {
    std::vector<Game::Command> ret;

    std::vector<Game::SearchResult> bfsresult;
    game.ReachableUnits(&bfsresult);
    int64_t max_score = std::numeric_limits<int64_t>::min();
    VLOG(1) << "next hands:" << bfsresult.size();
    for(const auto &res: bfsresult) {
      Game ng(game);
      int64_t score = 0;
      std::ostringstream os;
      if (ng.RunSequence(res.second)) {
        score = Score(ng, os);
      } else {
        score = MinScore(ng);
      }
      if (score > max_score) {
        VLOG(1) << "@" << res.first.pivot() << "-" << res.first.angle()
                << " score:" << max_score << " -> " << score;
        VLOG(1) << "scorer:" << os.str();
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
