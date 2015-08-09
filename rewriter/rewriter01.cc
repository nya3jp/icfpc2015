#include <gflags/gflags.h>
#include <glog/logging.h>
#include <picojson.h>

#include "json_parser.h"

DEFINE_string(problem, "", "Problem file.");
DEFINE_string(output, "", "Input solution.");

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logtostderr = true;

  picojson::value problem = ParseJson(FLAGS_problem);
  picojson::value output = ParseJson(FLAGS_output);

  int status = 0;
  for (const auto& entry: output.get<picojson::array>()) {
    CHECK_EQ(problem.get("id").get<int64_t>(),
             entry.get("problemId").get<int64_t>());
  }

#if 0
  int error_report = 0;
  for (const auto& entry : output.get<picojson::array>()) {
    CHECK_EQ(problem.get("id").get<int64_t>(),
             entry.get("problemId").get<int64_t>());
    int seed_index = FindIndex(
        problem.get("sourceSeeds").get<picojson::array>(),
        entry.get("seed").get<int64_t>());
    LOG(INFO) << "SeedIndex: " << seed_index;
    Game game;
    game.Load(problem, seed_index);
    LOG(INFO) << game;

    const std::string& solution = entry.get("solution").get<std::string>();
    bool is_finished = false;
    bool error = false;
    int i = 0;
    for (; i < solution.size(); ++i) {
      LOG(INFO) << CurrentState(game);
      Game::Command command = ParseCommand(solution[i]);
      LOG(INFO) << "Run: " << i << ", " << solution[i] << ", " << command;
      if (is_finished) {
        error = true;
        break;
      }
      if (!game.Run(command)) {
        is_finished = true;
      }
    }
    LOG(INFO) << "i: " << i << ", " << solution.size();
    error |= game.error();
    int score = error ? 0 : game.score();
    if (!error && FLAGS_enable_phrase_score) {
      score += PhraseScore(solution);
    }
    error_report |= error;
    std::cout << score << "\n";
  }

  return FLAGS_report_error && error_report;
#endif
  return 0;
}
