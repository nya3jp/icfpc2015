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

DEFINE_string(p, "ei!,r'lyeh,yuggoth,ia! ia!,necronomicon",
              "Power phrase");
DEFINE_bool(enable_phrase_score, false, "Enables the phrase scoring.");
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

Game::Command ParseCommand(char c) {
  switch(c) {
    case 'p': case '\'': case '!': case '.': case '0': case '3':
      return Game::Command::W;
    case 'b': case 'c': case 'e': case 'f': case 'y': case '2':
      return Game::Command::E;
    case 'a': case 'g': case 'h': case 'i': case 'j': case '4':
      return Game::Command::SW;
    case 'l': case 'm': case 'n': case 'o': case ' ': case '5':
      return Game::Command::SE;
    case 'd': case 'q': case 'r': case 'v': case 'z': case '1':
      return Game::Command::CW;
    case 'k': case 's': case 't': case 'u': case 'w': case 'x':
      return Game::Command::CCW;
    case '\t': case '\n': case '\r':
      return Game::Command::IGNORED;
    defult:
      LOG(FATAL) << "Unknown Character.";
  }
}

struct CurrentState {
  CurrentState(const Game& game) : game_(game) {
  }

  const Game& game_;
};

std::ostream& operator<<(std::ostream& os, const CurrentState& state) {
  state.game_.DumpCurrent(&os);
  return os;
}

}  // namespace

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logtostderr = true;

  picojson::value problem = ParseJson(FLAGS_problem);
  picojson::value output = ParseJson(FLAGS_output);
  std::vector<std::string> phrase_list = ParsePhraseList(FLAGS_p);

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
      score += PowerScore(solution, phrase_list);
    }
    error_report |= error;
    std::cout << score << "\n";
  }

  return FLAGS_report_error && error_report;
}
