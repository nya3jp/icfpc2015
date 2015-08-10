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


struct GameState {
  Game game;
  bool finished;
  int64_t score;
  std::string command0;
  GameState() {}
  GameState(const Game& game, bool finished, int64_t score)
    : game(game), finished(finished), score(score) {}
};

bool by_score_descend(const std::unique_ptr<GameState>& lhs,
                      const std::unique_ptr<GameState>& rhs) {
  return lhs->score > rhs->score;
}

DuralStarmanSolver::DuralStarmanSolver(GameScorer* scorer, int width, int depth)
  : scorer_(scorer), width_(width), depth_(depth) {}
DuralStarmanSolver::~DuralStarmanSolver() {}

std::string DuralStarmanSolver::NextCommands(const Game& game) {
  std::vector<std::unique_ptr<GameState>> prev_states;
  prev_states.emplace_back(new GameState(game, false, 0));
  std::string result_command;
  for (int d = 0; d <= depth_; ++d) {
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
        score = scorer_->Score(ng, f2, nullptr);  // TODO: debug
        std::unique_ptr<GameState> ngs(new GameState(ng, f2, score));
        if (d == 0) {
          ngs->command0 = Game::Commands2SimpleString(res.second);
        } else {
          ngs->command0 = p->command0;
        }
        next_states.emplace_back(std::move(ngs));
      }
    }
    sort(next_states.begin(), next_states.end(), by_score_descend);
    VLOG(1) << "d:" << d << ", width " << prev_states.size() << "->"
            << next_states.size() << ">>" << width_;
    if (next_states.size() > width_) {
      next_states.resize(width_);
    }
    prev_states.swap(next_states);
    if (!prev_states.empty()) {
      result_command = prev_states[0]->command0;
    } else {
      return result_command;
    }
  }
  return result_command;
}
