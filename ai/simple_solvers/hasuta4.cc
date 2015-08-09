#include <vector>

#include "../../simulator/game.h"
#include "../../simulator/solver.h"
#include "../../simulator/unit.h"

// Hasuta4: Hasuta3 + greedily erase line if possible
class Hasuta4 : public Solver {
public:
  Hasuta4() {}
  virtual ~Hasuta4() {}

  virtual std::string NextCommands(const Game& game) {
    std::vector<Game::Command> ret;
  
    std::vector<Game::SearchResult> bfsresult;
  
    game.ReachableUnits(&bfsresult);
  
    const Board &board = game.GetBoard();
    // Get point distribution
    std::vector<int> nfill(board.height());
    {
     
      for(int y = 0; y < board.height(); ++y)
        for(int x = 0; x < board.width(); ++x)
          if(board(x, y)) ++nfill[y];
    }
    
    {
      // Go greedily erase line if possible
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
  
    {
      // if impossible, try to add to denser line
      int targety = -1;
      // first 5 (yay heuristic) lines are unsafe
      std::vector<int>::const_iterator it = std::max_element(nfill.begin() + 5, nfill.end());
      if(it != nfill.end()) {
        targety = it - nfill.begin();
      }
      if(targety > 0) {
        int distx = 1 << 30;
        for(const auto &res: bfsresult) {
          const Unit &u = res.first;
          for(const auto &m: u.members()) {
            int dx = std::min(m.x(), game.GetBoard().width() - 1 - m.x());
            if(m.y() == targety && dx < distx) {
              ret = res.second;
              distx = dx;
            }
          }
        }
        if(distx < (1 << 30)) return Game::Commands2SimpleString(ret);
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
  return RunSolver(new Hasuta4(), "hasuta4.cc");
}

