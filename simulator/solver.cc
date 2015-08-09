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

int RunSolver(Solver* solver, std::string solver_tag) {
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

  // Record signal handler
  CHECK(std::signal(SIGUSR1, SigHandler) != SIG_ERR);
  CHECK(std::signal(SIGINT, SigHandler) != SIG_ERR);

  std::string final_commands;
  bool is_finished = false;
  bool error = false;
  Game game;
  game.Init(&game_data, 0);
  while(true) {
    VLOG(1) << game;
    // get sequence from AI
    const std::string instructions = solver->NextCommands(game);
    for(const auto& c: instructions) {
      final_commands.push_back(c);
      if (!game.Run(Game::Char2Command(c))) {
        is_finished = true;
      }
      if (is_finished) {
        break;
      }
    }
    if(is_finished || error) {
      break;
    }
    const int score = game.score();
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
  int score = error ? 0 : game.score();
  WriteOneJsonResult(problem.get("id").get<int64_t>(), solver_tag,
                     seed, score, final_commands);
  return 0;
}
