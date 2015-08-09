#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>

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
  Game game;
  game.Load(problem, 0);
  VLOG(1) << game;

  std::string final_commands;
  bool is_finished = false;
  bool error = false;
  int max_score = 0;
  while(true) {
    VLOG(1) << CurrentState(game);
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
    if (score > max_score) {
      max_score = score;
      WriteOneJsonResult(problem.get("id").get<int64_t>(), solver_tag,
                         seed, score, final_commands);
    }
  }
  int score = error ? 0 : game.score();
  WriteOneJsonResult(problem.get("id").get<int64_t>(), solver_tag,
                     seed, score, final_commands);
  return 0;
}
