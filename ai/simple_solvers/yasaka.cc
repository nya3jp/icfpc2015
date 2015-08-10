#include <vector>
#include <algorithm>
#include <tuple>

#include <glog/logging.h>

#include "../../simulator/game.h"
#include "../../simulator/solver.h"
#include "../../simulator/unit.h"
#include "../../simulator/ai_util.h"

using namespace std;

namespace {

enum EvalState {
  Unevaluated,
  Good,
  Bad
};

} // namespace

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
  
    vector<EvalState> isgood(bfsresult.size(), Unevaluated);
    // if impossible, try to add to denser, but solvable line
    int targety = get_target_y(game, nfill, isgood);
    VLOG(1) << "target y = " << targety;
    
    if(targety >= 0) {
      // if solvable, try to select the solution which places less items 
      int candidate = find_solutions_at(targety, game, bfsresult, &isgood);
      if(candidate >= 0) {
        VLOG(1) << "Candidate found";
        return Game::Commands2SimpleString(bfsresult[candidate].second);
      }
    }

    VLOG(1) << "No candidates found";

    // There can be two possiblities:
    // 1) target y == -1. this means it'll be game over very soon.
    // 2) target y >= 0. This means it is sovable but you need to place current unit somewhere else.
    // Basically, put items accordign to lowest (biggest) y and highest distx from good list,
    // but you need to be sure
    // that it doesn't hinder current unit placement path. (in the case of y >= 0)
    
    // score: (good_status_hack, smallest (highest) y, distance from wall). larger in tuple is the better
    typedef tuple<int, int, int> scoretype;
    vector<scoretype> scores(bfsresult.size(), make_tuple<int, int, int>(-1, -1, -1));
    for(size_t i = 0; i < bfsresult.size(); ++i) {
      const Game::SearchResult &res = bfsresult[i];
      const UnitLocation &u = res.first;
      // note high == smaller y
      int highesty = 1 << 30;
      int largestdx = -1;
      for(const auto &m: u.members()) {
        highesty = std::min(highesty, m.y());
        int dx = std::min(m.x(), game.GetBoard().width() - 1 - m.x());
        largestdx = std::max(largestdx, dx);
      }
      scores[i] = make_tuple(-1, highesty, largestdx);
    }

    if(targety == -1) {
      // survive mode, forget about targety
      const auto it = std::max_element(scores.begin(), scores.end());
      ssize_t d = it - scores.begin();
      return Game::Commands2SimpleString(bfsresult[d].second);
    }else {
      while(true) {
        const int invalid = -100; // hack
        const auto it = std::max_element(scores.begin(), scores.end());
        ssize_t d = it - scores.begin();
        if(std::get<0>(*it) == invalid) {
          return Game::Commands2SimpleString(bfsresult[d].second);
        }
        bool f = is_good(game, targety, bfsresult, d, &isgood);
        if(f) {
          return Game::Commands2SimpleString(bfsresult[d].second);
        }else{
          get<0>(*it) = invalid;
        }
      }
    }
    return Game::Commands2SimpleString(ret);
  }

private:

  // Get target Y line - dense line is preferred, but should be solvable
  int get_target_y(const Game &game, const vector<int> &nfill, vector<EvalState>& evalstate) {
    // first select appropriate lines
    Board reachable;
    GetReachabilityMapByAnyHands(game, &reachable);
    
    VLOG(2) << reachable;
    
    int ret;
    int placed = -1;
    for(int y = 0; y < reachable.height(); ++y) {
      int nreach = 0;
      for(int x = 0; x < reachable.width(); ++x) {
        if(reachable(x, y)) ++nreach;
      }
      if(nreach + nfill[y] != reachable.width()) continue;
      // now this line is solvable
      if(nfill[y] >= placed) { // prefers lower place in case of tie
        ret = y;
        placed = nfill[y];
      }
    }
    return ret;
  }

  // find solution that places at target y.
  // in case of ties solution placing more items on =y
  // in case of ties solution placing less items on <y
  // in case of ties solution placing nearer to walls get the highest score
  int find_solutions_at(int targety,
                        const Game &game,
                        const std::vector<Game::SearchResult>& candidates,
                        vector<EvalState>* evalstate) {
    typedef std::tuple<int, int, int> score;
    std::vector<score> scores;

    for(size_t i = 0; i < candidates.size(); ++i) {
      const UnitLocation &u = candidates[i].first;
      bool y_ok = false;
      int lessy = 0;
      int eqy = 0;
      int dist2wall = 1 << 30;
      for(const auto &m: u.members()) {
        if(m.y() < targety) {
          ++lessy;
        }else if(m.y() == targety) {
          ++eqy;
          int dx = std::min(m.x(), game.GetBoard().width() - 1 - m.x());
          dist2wall = std::min(dist2wall, dx);
        }
      }
      if(eqy > 0 && is_good(game, targety, candidates, i, evalstate)) {
        VLOG(2) << "candidate " << i << " pts: "  << lessy << " dist " << dist2wall;
        scores.push_back(std::make_tuple(-eqy, lessy, dist2wall));
      }else{
        scores.push_back(score(1 << 30, 1 << 30, 1 << 30));
      }
    }

    const auto& miniter = std::min_element(scores.begin(), scores.end());
    if(std::get<0>(*miniter) == 1 << 30) {
      return -1;
    }else{
      // XXX: this should be distance()
      return int(miniter - scores.begin());
    }
  }

  // Test whether candidate sequence hinders targety
  bool is_good(const Game &game, int targety,
               const std::vector<Game::SearchResult>& candidates,
               size_t candidatenum,
               vector<EvalState> *eval_state) 
  {
    CHECK(candidatenum < candidates.size());
    if((*eval_state)[candidatenum] == EvalState::Unevaluated) {
      const auto& c = candidates[candidatenum];
      Game newgame = game;
      newgame.RunSequence(c.second);
      Board reachability;
      (*eval_state)[candidatenum] = EvalState::Good;
      GetDotReachabilityFromTopAsMap(newgame, &reachability);
      for(int x = 0; x < game.GetBoard().width(); ++x) {
        if((!newgame.GetBoard()(x, targety)) && (!reachability(x, targety))) {
          (*eval_state)[candidatenum] = EvalState::Bad;
          break;
        }
      }
    }
    return (*eval_state)[candidatenum] == EvalState::Good;
  }
};

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logtostderr = true;
  return RunSolver(new Yasaka(), "Yasaka");
}

