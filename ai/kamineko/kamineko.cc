#include "kamineko.h"

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

#define ENABLE_DEBUG_LOG 0

DEFINE_int32(kamineko_hands, 3, "");

template<typename T>
std::string DumpV(const std::vector<T>& seq) {
  std::ostringstream ofs;
  for (const auto& e : seq) {
    ofs << e << ", ";
  }
  return ofs.str();
}

KaminekoScorer::KaminekoScorer() {}
KaminekoScorer::~KaminekoScorer() {}
int64_t KaminekoScorer::Score(const Game& game, bool finished,
                              std::string* debug) {
  const Board& board(game.board());
  if (finished) {
    const int64_t height = board.height();
    const int64_t width = board.width();
    return game.score() -
      2 * (height * width * height * 100 + height * width * 2000);
  }
  std::vector<int> height(GetHeightLine(game));
  int shade = 0;
  int block = 0;
  for (int i = 0; i < board.width(); ++i) {
    for (int j = height[i]; j < board.height(); ++j) {
      if (!board(i, j)) {
        shade ++; //= (j - height[i]) * (j - height[i]);
      } else {
        ++block;
      }
    }
  }

  int64_t height_score = GetHeightPenalty(height);
  int64_t result = game.score()
    - height_score * 100 - shade * 2000;
#if ENABLE_DEBUG_LOG
  if (debug) {
    std::ostringstream os;
    os << "height:" << DumpV(height) << "(score:" << height_score
       << " shade:" << shade
       << " score:" << game.score()
       << " total:" << result;
    *debug += os.str();
  }
#endif
  return result;
}

Kamineko::Kamineko()
  : scorer_(new KaminekoScorer()) {
  path_.reserve(FLAGS_kamineko_hands + 1);
}

Kamineko::Kamineko(GameScorer* scorer)
  : scorer_(scorer) {
  path_.reserve(FLAGS_kamineko_hands + 1);
}

Kamineko::~Kamineko() {}

static int64_t MinScore(const Game& game) {
  const Board& board(game.board());
  const int64_t height = board.height();
  const int64_t width = board.width();
  return game.score() -
    2 * (height * width * height * 100 + height * width * 2000);
}

std::unique_ptr<Kamineko::GamePath> AddNewPath(
    std::vector<std::unique_ptr<Kamineko::GamePath> >* pathp,
    std::unique_ptr<Kamineko::GamePath> next,
    int width) {
  std::vector<std::unique_ptr<Kamineko::GamePath> >& path = *pathp;
  if (path.size() < width) {
    path.emplace_back(std::move(next));
    return nullptr;
  }
  int min_index = 0;
  int min_score = path[0]->score;
  for (int i = 1; i < path.size(); ++i) {
    if (min_score > path[i]->score) {
      min_index = i;
      min_score = path[i]->score;
    }
  }
  if (min_score < next->score) {
    std::swap(path[min_index], next);
  }
  return next;
}

const Kamineko::GamePath& GetBest(
    const std::vector<std::unique_ptr<Kamineko::GamePath> >& path) {
  int max_index = 0;
  int max_score = path[0]->score;
  for (int i = 1; i < path.size(); ++i) {
    if (max_score < path[i]->score) {
      max_index = i;
      max_score = path[i]->score;
    }
  }
  return *path[max_index];
}

void Kamineko::AddGame(const Game& game) {
  path_.clear();
  path_.emplace_back(new GamePath(game, false, 0, "", ""));
}

bool Kamineko::Next(std::string* best_command, int* res_score) {
  std::vector<std::unique_ptr<Kamineko::GamePath> > next_path;
  for (const auto& p0 : path_) {
    if (p0->finished) {
      continue;
    }
    const Game& cur_game = p0->game;

    std::vector<Game::SearchResult> bfsresult;
    cur_game.ReachableUnits(&bfsresult);
    for (const auto &res : bfsresult) {
      Game ng(cur_game);
      std::string debug;
      bool finished = !ng.RunSequence(res.second);
#if ENABLE_DEBUG_LOG
      const int64_t score = scorer_->Score(ng, finished, &debug);
#else
      const int64_t score = scorer_->Score(ng, finished, nullptr);
#endif
      AddNewPath(
          &next_path,
          std::unique_ptr<Kamineko::GamePath>(new Kamineko::GamePath(
              ng, finished, score,
              p0->commands + Game::Commands2SimpleString(res.second),
              debug)),
          FLAGS_kamineko_hands);
    }
  }
  path_.swap(next_path);
  if (VLOG_IS_ON(1)) {
    for (const auto& p : path_) {
      VLOG(1) << "G#" << p->score
              << " \ngame:\n" << p->game
              << "\ndebug:" << p->debug;
    }
  }
  const Kamineko::GamePath& p = GetBest(path_);
  *res_score = p.game.score();
  *best_command = p.commands;
  return p.finished;
}
