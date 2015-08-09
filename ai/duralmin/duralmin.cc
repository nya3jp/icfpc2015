#include <algorithm>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "../../simulator/ai_util.h"
#include "../../simulator/board.h"
#include "../../simulator/game.h"
#include "../../simulator/hexpoint.h"
#include "../../simulator/solver.h"
#include "../../simulator/unit.h"

DEFINE_int32(duralmin_depth, 0, "depth");

template<typename T>
std::string DumpV(const std::vector<T>& seq) {
  std::ostringstream ofs;
  for (const auto& e : seq) {
    ofs << e << ", ";
  }
  return ofs.str();
}

class Duralmin : public Solver {
public:
  Duralmin() {}
  virtual ~Duralmin() {}

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

    int64_t height_score = GetHeightPenalty(height);
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

  typedef std::pair<HexPoint, int> UnitLocation;

  std::string DumpLocations(const std::vector<UnitLocation>& v) {
    std::ostringstream os;
    for (const auto& u : v) {
      os << u.first << "/" << u.second << ">";
    }
    return os.str();
  }

  int64_t Dfs(const Game& game, int depth,
              int64_t prev_max_score,
              std::vector<UnitLocation>* positions,
              std::string* result_command) {
    std::vector<Game::Command> ret;
    std::vector<Game::SearchResult> bfsresult;
    game.ReachableUnits(&bfsresult);
    int64_t max_score = std::numeric_limits<int64_t>::min();
    for(const auto &res: bfsresult) {
      positions->emplace_back(res.first.pivot(), res.first.angle());
      Game ng(game);
      int64_t score = 0;
      std::ostringstream os;
      if (ng.RunSequence(res.second)) {
        if (depth == 0) {
          score = Score(ng, os);
        } else {
          score = Dfs(ng, depth - 1, prev_max_score, positions, nullptr);
        }
      } else {
        score = MinScore(ng);
      }
      if (score > max_score) {
        ret = res.second;
        max_score = score;
        if (max_score > prev_max_score) {
          VLOG(1) << "@" << DumpLocations(*positions)
                  << " score:" << prev_max_score << " -> " << max_score;
          VLOG(1) << "scorer:" << os.str();
          prev_max_score = max_score;
        }
      }
      positions->pop_back();
    }
    if (result_command) {
      *result_command = Game::Commands2SimpleString(ret);
    }
    return max_score;
  }

  virtual std::string NextCommands(const Game& game) {
    std::string result;
    std::vector<UnitLocation> positions;
    Dfs(game, FLAGS_duralmin_depth, std::numeric_limits<int64_t>::min(),
        &positions, &result);
    return result;
  }
};

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logtostderr = true;
  return RunSolver(new Duralmin(), "Duralmin");
}
