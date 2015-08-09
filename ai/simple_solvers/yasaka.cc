#include <vector>
#include <algorithm>

#include <glog/logging.h>

#include "../../simulator/game.h"
#include "../../simulator/solver.h"
#include "../../simulator/unit.h"
#include "../../simulator/ai_util.h"

// yasaka: Hasuta4 + reachability check
class Yasaka : public Solver {
public:
  Yasaka() {}
  virtual ~Yasaka() {}

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
        const UnitLocation &u = res.first;
        for(const auto &m: u.members()) {
          nfill_copy[m.y()]++;
          if(nfill_copy[m.y()] == board.width())
            return Game::Commands2SimpleString(res.second);
        }
      }
    }
  
    {
      // if impossible, try to add to denser, but solvable line

      // first select appropriate lines
      int targety = -1;
      Board reachable;
      GetReachabilityMapByAnyHands(game, &reachable);

      VLOG(2) << reachable;

      int placed = -1;
      for(int y = 0; y < reachable.height(); ++y) {
        int nreach = 0;
        for(int x = 0; x < reachable.width(); ++x) {
          if(reachable(x, y)) ++nreach;
        }
        if(nreach + nfill[y] != reachable.width()) continue;
        // now this line is solvable
        if(nfill[y] > placed) {
          targety = y;
          placed = nfill[y];
        }
      }
      VLOG(1) << "target y = " << targety;

      // if solvable, try to select the solution which places less items 
      int candidate = find_solutions_at(targety, game, bfsresult);
      if(candidate >= 0) {
        VLOG(1) << "Candidate found";
        return Game::Commands2SimpleString(bfsresult[candidate].second);
      }
      VLOG(1) << "No candidates found";
    }

    // otherwise, put in lowest y and lowest distx
    int distx = 1 << 30;
    int maxy = -1;
    for(const auto &res: bfsresult) {
      const UnitLocation &u = res.first;
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

private:
  // find solution that places at target y.
  // in case of ties solution placing less items on <y
  // in case of ties solution placing nearer to walls get the highest score
  int find_solutions_at(int targety,
                        const Game &game,
                        const std::vector<Game::SearchResult>& candidates) {
    typedef std::pair<int, int> score;
    std::vector<score> scores;

    for(size_t i = 0; i < candidates.size(); ++i) {
      const UnitLocation &u = candidates[i].first;
      int lessy = 0;
      int dist2wall = 1 << 30;
      for(const auto &m: u.members()) {
        if(m.y() < targety) {
          ++lessy;
        }else if(m.y() == targety) {
          int dx = std::min(m.x(), game.GetBoard().width() - 1 - m.x());
          dist2wall = std::min(dist2wall, dx);
        }
      }
      VLOG(2) << "candidate " << i << " pts: "  << lessy << " dist " << dist2wall;
      scores.push_back(std::make_pair(lessy, dist2wall));
    }

    
    const auto& miniter = std::min_element(scores.begin(), scores.end());
    if(miniter->second == 1 << 30) {
      return -1;
    }else{
      // XXX: this should be distance()
      return int(miniter - scores.begin());
    }
  }
};

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logtostderr = true;
  return RunSolver(new Yasaka(), "Yasaka");
}

