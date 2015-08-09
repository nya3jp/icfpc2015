#include <gflags/gflags.h>
#include <glog/logging.h>
#include <picojson.h>

#include "json_parser.h"
#include "game_json_util.h"
#include "game.h"

DEFINE_string(problem, "", "Problem file.");
DEFINE_string(output, "", "Input solution.");

namespace {

int FindIndex(const picojson::array& source_seeds, int64_t seed) {
  for (int i = 0; i < source_seeds.size(); ++i) {
    if (source_seeds[i].get<int64_t>() == seed) {
      return i;
    }
  }
  return -1;
}

}  // namespace

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logtostderr = true;

  picojson::value problem = ParseJson(FLAGS_problem);
  picojson::value output = ParseJson(FLAGS_output);

  picojson::array rewritten_output;
  int status = 0;
  for (const auto& entry: output.get<picojson::array>()) {
    CHECK_EQ(problem.get("id").get<int64_t>(),
             entry.get("problemId").get<int64_t>());
    int seed_index = FindIndex(
        problem.get("sourceSeeds").get<picojson::array>(),
        entry.get("seed").get<int64_t>());
    Game game;
    game.Load(problem, seed_index);

    const std::string& solution = entry.get("solution").get<std::string>();
    std::string rewritten_solution;
    for (size_t i = 0; i < solution.size(); ++i) {
      LOG(INFO) << CurrentState(game);
      Game::Command command = ParseCommand(solution[i]);
      if (command == Game::Command::SW) {
        bool is_possible = true;
        for (size_t j = i + 1; j < solution.size(); ++j) {
          Game::Command c = ParseCommand(solution[j]);
          if (c == Game::Command::E) {
            is_possible = false;
            break;
          }
          if (c == Game::Command::IGNORED ||
              c == Game::Command::CW ||
              c == Game::Command::CCW) {
            continue;
          }
          break;
        }
        if (is_possible) {
          Game dryrun = game;
          int current_index = game.current_index();
          dryrun.Run(Game::Command::E);  // e
          dryrun.Run(Game::Command::SW);  // a
          dryrun.Run(Game::Command::W);  // !
          is_possible =
              !dryrun.error() && current_index == dryrun.current_index();
        }
        rewritten_solution += "ea!";
      } else {
        rewritten_solution.push_back(solution[i]);
      }

      // TODO
      game.Run(command);
    }
    rewritten_output.emplace_back(BuildOutputEntry(
        entry.get("problemId").get<int64_t>(),
        entry.get("seed").get<int64_t>(),
        entry.get("tag").get<std::string>(),
        rewritten_solution));
  }
  std::cout << picojson::value(rewritten_output) << "\n";
  return 0;
}
