#include "duralstarman.h"

#include <algorithm>
#include <limits>
#include <memory>
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

DuralStarmanScorer::DuralStarmanScorer(GameScorer* parent, int width, int depth)
  : parent_(parent), depth_(depth), width_(width) {}
DuralStarmanScorer::~DuralStarmanScorer() {}

struct GameState {
  Game game;
  bool finished;
  int64_t score;
  GameState() {}
  GameState(const Game& game, bool finished, int64_t score)
    : game(game), finished(finished), score(score) {}
};

bool by_score_descend(const std::unique_ptr<GameState>& lhs,
                      const std::unique_ptr<GameState>& rhs) {
  return lhs->score > rhs->score;
}

int64_t DuralStarmanScorer::Score(const Game& game, bool finished,
                                  std::string* debug) {
  if (depth_ == 0 || finished) {
    return parent_->Score(game, finished, debug);
  }
  std::vector<std::unique_ptr<GameState>> prev_states;
  prev_states.emplace_back(new GameState(game, finished, 0));
  for (int d = 0; d < depth_; ++d) {
    std::vector<std::unique_ptr<GameState>> next_states;
    for (const auto& p : prev_states) {
      if (p->finished) {
        continue;
      }
      Game cur_game(p->game);

      std::vector<Game::SearchResult> bfsresult;
      cur_game.ReachableUnits(&bfsresult);
      for (const auto& res : bfsresult) {
        Game ng(cur_game);
        bool f2 = !ng.RunSequence(res.second);
        std::string nd;
        int64_t score;
        if (debug) {
          score = parent_->Score(ng, f2, &nd);
        } else {
          score = parent_->Score(ng, f2, nullptr);
        }
        next_states.emplace_back(new GameState(ng, f2, score));
      }
    }
    sort(next_states.begin(), next_states.end(), by_score_descend);
    VLOG(1) << "d:" << d << ", width " << prev_states.size() << "->"
            << next_states.size() << ">>" << width_;
    next_states.resize(width_);
    prev_states.swap(next_states);
  }
  return prev_states[0]->score;
}
