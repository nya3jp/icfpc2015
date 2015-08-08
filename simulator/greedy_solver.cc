#include <vector>

#include "game.h"
#include "solver.h"
#include "unit.h"


class GreedySolver : public Solver {
public:
  GreedySolver() {}
  virtual ~GreedySolver() {}

  virtual std::vector<Game::Command> NextCommands(const Game& game) {
    std::vector<Game::Command> ret;

    std::vector<Game::SearchResult> bfsresult;
    int maxy = -1;
    game.ReachableUnits(&bfsresult);
    for(const auto &res: bfsresult) {
      const Unit &u = res.first;
      for(const auto &m: u.members()) {
        if(m.y() > maxy) {
          ret = res.second;
          maxy = m.y();
        }
      }
    }
    return ret;
  }
};

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logtostderr = true;
  return RunSolver(new GreedySolver());
}
