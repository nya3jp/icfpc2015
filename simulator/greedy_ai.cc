#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <picojson.h>

#include "game.h"

DEFINE_string(problem, "", "problem file");
DEFINE_string(result, "", "result file");

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

std::vector<Game::Command> get_greedy_instructions(Game &game)
{
  std::vector<Game::Command> ret;

  std::vector<Game::SearchResult> bfsresult;
  int maxy = -1;
  game.ReachableUnits(&bfsresult);
  for(const auto &res: bfsresult) {
    const Unit &u = res.first;
    for(const auto &m: u.members()) {
      if(m.y() > maxy) {
        ret = res.second;
        maxy = m.y();
      }
    }
  }
  return ret;
}

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logtostderr = true;

  picojson::value problem;
  {
    std::ifstream stream(FLAGS_problem);
    stream >> problem;
    CHECK(stream.good()) << picojson::get_last_error();
  }
 
  int seed_index = 0;
  LOG(INFO) << "SeedIndex: " << seed_index;
  Game game;
  game.Load(problem, seed_index);
  LOG(INFO) << game;

  bool is_finished = false;
  bool error = false;
  while(true) {
    LOG(INFO) << CurrentState(game);
    // get sequence from AI
    std::vector<Game::Command> instructions = get_greedy_instructions(game);
    for(const auto& c: instructions) {
      std::cerr << c << " ";
    }
    std::cerr << std::endl;
    
    for(const auto& c: instructions) {
      if (!game.Run(c)) {
        is_finished = true;
      }
      
      if (is_finished) {
        error = true;
        break;
      }
    }
    if(error) {
      break;
    }
  }
  int score = error ? 0 : game.score();
  std::cout << score << "\n";
  
  // TODO: write JSON
  return 0;
}
