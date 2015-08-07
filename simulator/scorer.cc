#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <picojson.h>

#include "game.h"

DEFINE_string(problem, "", "problem file");
DEFINE_string(output, "", "output file");

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

}  // namespace

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  picojson::value problem;
  {
    std::ifstream stream(FLAGS_problem);
    stream >> problem;
    CHECK(stream.good()) << picojson::get_last_error();
  }
  picojson::value output;
  {
    std::ifstream stream(FLAGS_output);
    stream >> output;
    CHECK(stream.good()) << picojson::get_last_error();
  }

  for (const auto& entry : output.get<picojson::array>()) {
    CHECK_EQ(problem.get("id").get<int64_t>(),
             entry.get("problemId").get<int64_t>());
    int seed_index = FindIndex(
        problem.get("sourceSeeds").get<picojson::array>(),
        entry.get("seed").get<int64_t>());
    LOG(ERROR) << "SeedIndex: " << seed_index;
    Game game;
    game.Load(problem, seed_index);
    LOG(ERROR) << game;

    const std::string& solution = entry.get("solution").get<std::string>();
    int i = 0;
    for (; i < solution.size(); ++i) {
      game.DumpCurrent(&std::cerr);
      std::cerr << "\n";
      Game::Command command = ParseCommand(solution[i]);
      if (!game.Run(command)) {
        break;
      }
    }

    int score = (i < solution.size()) ? 0 : game.score();
    std::cout << score << "\n";
  }
}
