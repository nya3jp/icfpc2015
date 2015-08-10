#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>

#include <csignal>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <picojson.h>

#include "game.h"
#include "solver.h"

DEFINE_string(ai_tag, "", "Tag of this trial");
DEFINE_string(p, "", "comma-separated power phrases");

namespace {

void WriteOneJsonResult(int problemid,
                        const std::string& tag,
                        int64_t seed,
                        int score,
                        const std::string& commands) {

  picojson::object output;
  output["solution"] = picojson::value(commands);
  output["problemId"] = picojson::value((int64_t)problemid);
  output["seed"] = picojson::value((int64_t)seed);
  output["tag"] = picojson::value(tag);
  output["_score"] = picojson::value((int64_t)score);

  std::vector<picojson::value> outputs;
  outputs.emplace_back(picojson::value(output));

  picojson::value finstr(outputs);
  std::cout << finstr.serialize() << std::endl;
}

}  // namespace

Solver::Solver() {}
Solver::~Solver() {}

Solver2::Solver2() {}
Solver2::~Solver2() {}

GameScorer::GameScorer() {}
GameScorer::~GameScorer() {}

class Solver12Converter : public Solver2 {
public:
  Solver12Converter(Solver* solver)
    : solver_(solver) {}
  ~Solver12Converter() {}

  virtual void AddGame(const Game& game) {
    game_ = game;
    final_commands_ = "";
  }

  virtual bool Next(std::string* best_command, int* score) {
    VLOG(1) << game_;
    // get sequence from AI
    bool is_finished = false;
    const std::string instructions = solver_->NextCommands(game_);
    for(const auto& c: instructions) {
      final_commands_.push_back(c);
      if (!game_.Run(Game::Char2Command(c))) {
        is_finished = true;
      }
      if (is_finished) {
        break;
      }
    }
    if (score) {
      *score = game_.score();
    }
    if (best_command) {
      *best_command = final_commands_;
    }
    return is_finished;
  }
private:
  Solver* solver_;
  Game game_;
  std::string final_commands_;
};

Solver2* ConvertS12(Solver* solver) {
  return new Solver12Converter(solver);
}

volatile sig_atomic_t sig_sig_ = 0;

void SigHandler(int num) {
  switch(num) {
  case SIGUSR1:
    sig_sig_ = 1;
    break;
  case SIGINT:
    sig_sig_ =2;
    break;
  default:
    break;
  }
}

// TODO: make this a wrapper of RunSolver2.
int RunSolver(Solver* solver, std::string solver_tag) {
  Solver2 *s2 = ConvertS12(solver);
  RunSolver2(s2, solver_tag);
  delete s2;
  return 0;
}

int RunSolver2(Solver2* solver, std::string solver_tag) {
  if (!FLAGS_ai_tag.empty()) {
    solver_tag = FLAGS_ai_tag;
  }
  picojson::value problem;
  // Read problem from stdin.
  {
    std::cin >> problem;
    CHECK(std::cin.good()) << picojson::get_last_error();
  }

  // Always get the #0 seed.
  const int64_t seed =
    problem.get("sourceSeeds").get<picojson::array>()[0].get<int64_t>();
  VLOG(1) << " Seed: " << seed;
  GameData game_data;
  game_data.Load(problem);
  VLOG(1) << game_data;

  {
    Game game;
    game.Init(&game_data, 0);
    solver->AddGame(game);
  }
  // Record signal handler
  CHECK(std::signal(SIGUSR1, SigHandler) != SIG_ERR);
  CHECK(std::signal(SIGINT, SigHandler) != SIG_ERR);

  std::string final_commands;
  int score;
  while(true) {
    bool is_finished = solver->Next(&final_commands, &score);
    if(is_finished) {
      break;
    }
    if (sig_sig_ == 2) {
      VLOG(1) << "Shutting down:" << seed << ", " << score;
      break;
    }
    if (sig_sig_ == 1) {
      VLOG(1) << "Dump Result:" << seed << ", " << score;
      WriteOneJsonResult(problem.get("id").get<int64_t>(), solver_tag,
                         seed, score, final_commands);
      sig_sig_ = 0;
    }
  }
  WriteOneJsonResult(problem.get("id").get<int64_t>(), solver_tag,
                     seed, score, final_commands);
  return 0;
}
