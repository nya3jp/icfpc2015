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

  GameData game_data;
  game_data.Load(parsed);
  LOG(ERROR) << game_data;

  Game game;
  game.Init(&game_data, 0);  // TODO seed_index.

  // TODO
  while (true) {
    LOG(INFO) << game;
    if (!game.Run(Game::Command::SE)) {
      break;
    }
  }
  LOG(ERROR) << "Score: " << game.score();

  return 0;
}
