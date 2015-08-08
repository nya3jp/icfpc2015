#include <iostream>
#include <fstream>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <picojson.h>

#include "game.h"

DEFINE_string(f, "", "input file");


int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  picojson::value parsed;
  {
    std::ifstream stream(FLAGS_f);
    stream >> parsed;
    CHECK(stream.good()) << picojson::get_last_error();
  }
  LOG(INFO) << parsed;

  Game game;
  game.Load(parsed, 0);  // TODO seed_index.
  LOG(ERROR) << game;

  std::vector<Game::SearchResult> units;
  std::cerr << "BFS start" << std::endl;
  game.ReachableUnits(&units);
  std::cerr << "Reachable areas:" << units.size() << std::endl;
  for (const auto& res : units) {
    std::cerr << "@" << res.first.pivot()
              << " " << res.first.angle()
              << " :";
    for (const auto& c : res.second) {
      std::cerr << c << ",";
    }
    std::cerr << std::endl;
  }

  // TODO
  while (true) {
    game.DumpCurrent(&std::cerr);
    std::cerr << "\n";
    if (!game.Run(Game::Command::SE)) {
      break;
    }
  }
  LOG(ERROR) << "Score: " << game.score();

  return 0;
}
