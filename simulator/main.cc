#include <iostream>
#include <fstream>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <picojson.h>

#include "board.h"

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

  Board board;
  board.Load(parsed);
  LOG(ERROR) << "\n" << board;

  return 0;
}
