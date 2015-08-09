#include <vector>

#include "../../simulator/game.h"
#include "../../simulator/solver.h"
#include "../../simulator/unit.h"


class GreedySolver2 : public Solver {
public:
  GreedySolver2() {}
  virtual ~GreedySolver2() {}

  virtual std::string NextCommands(const Game& game) {
    std::vector<Game::Command> ret;
  
    std::vector<Game::SearchResult> bfsresult;
    int distx = 1 << 30;
    int maxy = -1;
    game.ReachableUnits(&bfsresult);
    for(const auto &res: bfsresult) {
      const Unit &u = res.first;
      for(const auto &m: u.members()) {
        int dx = std::min(m.x(), game.GetBoard().width() - 1 - m.x());
        if(m.y() > maxy || (m.y() == maxy && dx < distx)) {
          ret = res.second;
          maxy = m.y();
          distx = dx;
        }
      }
    }
    return Game::Commands2SimpleString(ret);
  }
};

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logtostderr = true;
  return RunSolver(new GreedySolver2(), "greedy_ai_2.cc");
}

