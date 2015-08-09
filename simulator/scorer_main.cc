#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <picojson.h>

#include "game.h"
#include "json_parser.h"
#include "scorer.h"

DEFINE_string(problem, "", "problem file");
DEFINE_string(output, "", "output file");

DEFINE_string(p, "ei!,r'lyeh,yuggoth,ia! ia!,necronomicon,yogsothoth",
              "Power phrase");
DEFINE_bool(report_error, false,
            "Returns non-zero status code if an error is found in any case.");

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
  std::vector<std::string> phrase_list = ParsePhraseList(FLAGS_p);

  int status = 0;
  for (const auto& entry : output.get<picojson::array>()) {
    CHECK_EQ(problem.get("id").get<int64_t>(),
             entry.get("problemId").get<int64_t>());
    int seed_index = FindIndex(
        problem.get("sourceSeeds").get<picojson::array>(),
        entry.get("seed").get<int64_t>());
    LOG(INFO) << "SeedIndex: " << seed_index;
    GameData game_data;
    game_data.Load(problem);
    LOG(INFO) << game_data;

    Game game;
    game.Init(&game_data, seed_index);
    const std::string& solution = entry.get("solution").get<std::string>();
    for (size_t i = 0; i < solution.size(); ++i) {
      LOG(INFO) << CurrentState(game);
      Game::Command command = ParseCommand(solution[i]);
      LOG(INFO) << "Run: " << i << ", " << solution[i] << ", " << command;
      game.Run(command);
    }
    int error = game.error();
    int score = error ? 0 : game.score();
    if (!error) {
      score += PowerScore(solution, phrase_list);
    }
    std::cout << score << "\n";

    status |= error;
  }

  return FLAGS_report_error ? status : 0;
}
