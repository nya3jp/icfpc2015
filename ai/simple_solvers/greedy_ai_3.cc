#include <vector>

#include "../../simulator/game.h"
#include "../../simulator/solver.h"
#include "../../simulator/unit.h"


class GreedySolver3 : public Solver {
public:
  GreedySolver3() {}
  virtual ~GreedySolver3() {}

  virtual std::string NextCommands(const Game& game) {
    std::vector<Game::Command> ret;
  
    std::vector<Game::SearchResult> bfsresult;
  
    game.ReachableUnits(&bfsresult);
  
    // Go greedily erase line if possible
    {
      const Board &board = game.GetBoard();
      std::vector<int> nfill(board.height());
      for(int y = 0; y < board.height(); ++y)
        for(int x = 0; x < board.width(); ++x)
          if(board(x, y)) ++nfill[y];
      
      for(const auto &res: bfsresult) {
        std::vector<int> nfill_copy(nfill);
        const Unit &u = res.first;
        for(const auto &m: u.members()) {
          nfill_copy[m.y()]++;
          if(nfill_copy[m.y()] == board.width())
            return Game::Commands2SimpleString(res.second);
        }
      }
    }
  
    int distx = 1 << 30;
    int maxy = -1;
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
  return RunSolver(new GreedySolver3(), "greedy_ai_3.cc");
}
