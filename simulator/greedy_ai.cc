#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <picojson.h>

#include "game.h"

DEFINE_int64(problemid, -1, "problemId");
DEFINE_string(problem, "", "problem file");

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

std::string encode_command(const std::vector<Game::Command>& commands)
{
  std::string ret;
  for(const auto &c: commands) {
    switch(c) {
    case Game::Command::W:
      ret += '!';
      break;
    case Game::Command::E:
      ret += 'e';
      break;
    case Game::Command::SW:
      ret += 'i';
      break;
    case Game::Command::SE:
      ret += ' ';
      break;
    case Game::Command::CW:
      ret += 'd';
      break;
    case Game::Command::CCW:
      ret += 'k';
      break;
    default:
      LOG(FATAL) << "Unknown command " << c;
    }
  }
  return ret;
}

void write_json(int problemid,
                int64_t seed,
                const std::string& tag,
                int score,
                const std::vector<Game::Command>& commands)
{
  picojson::object output;

  picojson::value solution(encode_command(commands));
  output["solution"] = solution;
  output["problemId"] = picojson::value((int64_t)problemid);
  output["seed"] = picojson::value((int64_t)seed);
  output["tag"] = picojson::value(tag);
  output["_score"] = picojson::value((int64_t)score);

  std::vector<picojson::value> outputs;
  outputs.emplace_back(picojson::value(output));
  picojson::value finstr(outputs);
  std::cout << finstr.serialize();
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
  
  CHECK(FLAGS_problemid >= 0);

  int seed_index = 0;
  int64_t seed = problem.get("sourceSeeds").get<picojson::array>()[seed_index].get<int64_t>();
  LOG(INFO) << "SeedIndex: " << seed_index << " Seed: " << seed;
  Game game;
  game.Load(problem, seed_index);
  LOG(INFO) << game;

  std::vector<Game::Command> final_commands;
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
      final_commands.push_back(c);
      if (!game.Run(c)) {
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
  
  write_json(FLAGS_problemid, seed, __FILE__, score, final_commands);
  
  return 0;
}
