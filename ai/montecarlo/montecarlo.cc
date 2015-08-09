#include <random>

#include "../../simulator/solver.h"
#include "../../simulator/common.h"


DEFINE_int64(seed, 178116, "");
DEFINE_int32(iteration, 15, "");

namespace {

class MontecarloSolver : public Solver {
 public:
  MontecarloSolver(int seed, int iteration)
      : current_(0), rand_(seed), iteration_(iteration) {}
  virtual ~MontecarloSolver() {}

  virtual std::string NextCommands(const Game& game) override {
    LOG(ERROR) << "Current: " << current_;
    ++current_;

    std::vector<Game::SearchResult> candidate_list;
    game.ReachableUnits(&candidate_list);
    int max_score = 0;
    const Game::SearchResult* result = nullptr;
    for (const auto& candidate : candidate_list) {
      int score = Run(game, candidate.second);
      if (score > max_score) {
        max_score = score;
        result = &candidate;
      }
    }
    return Game::Commands2SimpleString(result->second);
  }

 private:
  int Run(const Game& original_game,
          const std::vector<Game::Command>& commands) {
    Game base_game = original_game;
    if (!base_game.RunSequence(commands)) {
      return base_game.score();
    }

    // Height score.
    int height_score = 0;
    {
      const Board::Map& cells = base_game.GetBoard().cells();
      for (size_t x = 0; x < base_game.GetBoard().width(); ++x) {
        for (size_t y = 0; y < base_game.GetBoard().height(); ++y) {
          if (cells[y][x]) {
            height_score += y * y;
          }
        }
      }
    }

    // Row cell score.
    int row_score = 0;
    {
      const Board::Map& cells = base_game.GetBoard().cells();
      for (size_t y = 0; y < cells.size(); ++y) {
        int count = 0;
        for (int i : cells[y]) {
          count += i;
        }
        row_score += count * count;
      }
    }

    int total_score = 0;
    for (int i = 0; i < iteration_; ++i) {
      Game dryrun = base_game;
      while (true) {
        std::vector<Game::SearchResult> candidate_list;
        dryrun.ReachableUnits(&candidate_list);
        std::uniform_int_distribution<> dist(0, candidate_list.size() - 1);
        const Game::SearchResult& chosen = candidate_list[dist(rand_)];
        if (!dryrun.RunSequence(chosen.second)) {
          break;
        }
      }
      total_score += dryrun.score();
    }
    return total_score + height_score + row_score;
  }

  int current_;
  std::mt19937 rand_;
  int iteration_;
  DISALLOW_COPY_AND_ASSIGN(MontecarloSolver);
};

}  // namespace

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logtostderr = true;
  LOG(INFO) << "Montecarlo Seed: " << FLAGS_seed;
  return RunSolver(new MontecarloSolver(FLAGS_seed, FLAGS_iteration),
                   "Montecarlo");
}
