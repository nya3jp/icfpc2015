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

DEFINE_int64(seedindex, -1, "seedindex");
DEFINE_string(problem, "", "problem file");
DEFINE_string(ai_tag, __FILE__, "Tag of this trial");

namespace {

struct CurrentState {
  CurrentState(const Game& game) : game_(game) {
  }

  const Game& game_;
};

std::ostream& operator<<(std::ostream& os, const CurrentState& state) {
  state.game_.DumpCurrent(&os);
  return os;
}

struct resultseq
{
  int64_t seed;
  int score;
  std::string commands;

  resultseq(){}
  resultseq(int64_t seed, int score, const std::string commands)
    : seed(seed), score(score), commands(commands) {}
};

void write_json(int problemid,
                const std::string& tag,
                const std::vector<resultseq>& seeds_and_commands)
{
  std::vector<picojson::value> outputs;
  for(const auto &s: seeds_and_commands) {
    int64_t seed = s.seed;
    int score = s.score;

    picojson::object output;

    output["solution"] = picojson::value(s.commands);
    output["problemId"] = picojson::value((int64_t)problemid);
    output["seed"] = picojson::value((int64_t)seed);
    output["tag"] = picojson::value(tag);
    output["_score"] = picojson::value((int64_t)score);

    outputs.emplace_back(picojson::value(output));
  }
  picojson::value finstr(outputs);
  std::cout << finstr.serialize();
}

}  // namespace

Solver::Solver() {}
Solver::~Solver() {}

int RunSolver(Solver* solver) {
  picojson::value problem;
  {
    std::ifstream stream(FLAGS_problem);
    stream >> problem;
    CHECK(stream.good()) << picojson::get_last_error();
  }
  
  std::vector<int64_t> seed_indices;
  
  if(FLAGS_seedindex >= 0) {
    seed_indices.push_back(FLAGS_seedindex);
  } else {
    for(size_t i = 0;
        i < problem.get("sourceSeeds").get<picojson::array>().size();
        ++i) {
      seed_indices.push_back(i);
    }
  }

  std::vector<resultseq> seeds_and_results;

  for(const auto& seed_index: seed_indices) {
    int64_t seed = problem.get("sourceSeeds").get<picojson::array>()[seed_index].get<int64_t>();
    LOG(INFO) << "SeedIndex: " << seed_index << " Seed: " << seed;
    Game game;
    game.Load(problem, seed_index);
    LOG(INFO) << game;

    std::string final_commands;
    bool is_finished = false;
    bool error = false;
    while(true) {
      LOG(INFO) << CurrentState(game);
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
    }
    int score = error ? 0 : game.score();
    std::cerr << score << "\n";

    seeds_and_results.emplace_back(resultseq(seed, score, final_commands));
  }
  write_json(problem.get("id").get<int64_t>(), FLAGS_ai_tag,
             seeds_and_results);
  return 0;
}
