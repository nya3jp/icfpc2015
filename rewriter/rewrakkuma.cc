#include <cassert>
#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <algorithm>
#include <set>
#include <queue>
#include <random>
#include <signal.h>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <picojson.h>

#include "../../simulator/board.h"
#include "../../simulator/game.h"
#include "../../simulator/unit.h"

/////////////////////////////////////////////////////////////////////////////////
// Signal handling.
/////////////////////////////////////////////////////////////////////////////////

bool HURRY_UP_MODE = false;

void set_hurry_up_mode(int signum) {
  HURRY_UP_MODE = true;
}

void install_signal_handlers() {
  signal(SIGUSR1, &set_hurry_up_mode);
  signal(SIGINT, &set_hurry_up_mode);
}

/////////////////////////////////////////////////////////////////////////////////
// Super tenuki utilities.
/////////////////////////////////////////////////////////////////////////////////

int count_occurrence(const std::string& heystack, const std::string& needle) {
  int cnt = 0;
  for(int s=0; s+needle.size()<=heystack.size(); ++s)
    if(needle == heystack.substr(s, needle.size()))
      ++cnt;
  return cnt;
}

int score(const std::string& cmd, const std::vector<std::string>& phrases) {
  int total_score = 0;
  for(auto& p: phrases) {
    int resp = count_occurrence(cmd, p);
    int lenp = p.size();
    total_score += 2 * lenp * resp + (resp ? 300 : 0);
  }
  return total_score;
}

const char* g_cmds[] = {
  "p'!.03", // W
  "bcefy2", // E
  "aghij4", // SW
  "lmno 5", // SE
  "dqrvz1", // RC
  "kstuwx", // RCC
};
std::mt19937 g_rand;

std::vector<std::string> random_default_moves() {
  //Better not to fall down to try many passes.
  std::vector<int> idx = {4,5,0,1,2,3};
  //std::shuffle(idx.begin(), idx.end(), g_rand); 

  std::vector<std::string> result;
  for (int i=0; i<6; ++i) {
    result.emplace_back(1,
       g_cmds[idx[i]][std::uniform_int_distribution<int>(0,5)(g_rand)]);
  }
  return result;
}

/////////////////////////////////////////////////////////////////////////////////
// On-graph solver.
//   Find a way on |Graph| from |Start| to |Goal|.
//   Let's incude |phrases| as many as possible.
/////////////////////////////////////////////////////////////////////////////////

typedef int Vert;
typedef std::pair<Game::Command, Vert> Edge;
typedef std::vector<Edge> Edges;
typedef std::vector<Edges> Graph;

std::vector<bool> calc_goal_reachability(const Graph& g, Vert goal) {
  Graph r(g.size());
  for (int v=0; v<g.size(); ++v)
    for (auto& cu: g[v])
      r[cu.second].emplace_back(cu.first, v);

  std::vector<bool> visited(g.size()); visited[goal]=true;
  std::queue<Vert> Q; Q.push(goal);
  while (!Q.empty()) {
    Vert v = Q.front(); Q.pop();
    for (auto& cu: r[v]) {
      Vert u = cu.second;
      if (!visited[u]) {
         visited[u] = true;
         Q.push(u);
      }
    }
  }
  return visited;
}

std::string solve_on_graph(
    const std::string& hint,
    const Graph& Graph,
    const std::vector<int>& depth,
    Vert Start,
    Vert Goal,
    std::vector<std::string> phrases) {
  std::string result;

  const std::vector<bool> reachable_to_goal = calc_goal_reachability(Graph, Goal);

  std::vector<bool> visited(Graph.size());
  Vert cur = Start;
  visited[cur] = true;
  while (cur != Goal) {
    if (HURRY_UP_MODE)
      return hint;
    auto is_goalable = [&]() {
      auto V = visited;
      std::queue<Vert> Q; Q.push(cur);
      while (!Q.empty()) {
        if (HURRY_UP_MODE)
          return false;
        Vert v = Q.front(); Q.pop();
        if (v == Goal)
          return true;
        if (depth[cur]<depth[v] && reachable_to_goal[v])
          return true;
        for (auto cmd_next: Graph[v]) {
          Vert u = cmd_next.second;
          if (V[u]) continue;
          V[u] = true;
          Q.push(u);
        }
      }
      return false;
    };
    auto walk_by = [&](const std::string& s) {
      for (char c: s) {
        bool found = false;
        for (auto& cmd_next: Graph[cur])
          if (cmd_next.first == Game::Char2Command(c) &&
              !visited[cmd_next.second]) {
            cur = cmd_next.second;
            visited[cur] = true;
            found = true;
            break;
          }
        if(!found)
          return false;
      }
      return true;
    };
    auto try_phrase = [&](const std::string& ph) {
      auto snapshot_visited = visited;
      Vert snapshot_cur = cur;
      if (!walk_by(ph) || !is_goalable()) {
        cur = snapshot_cur;
        visited = std::move(snapshot_visited);
        return false;
      }
      result += ph;
      return true;
    };
    auto try_phrases = [&](std::vector<std::string>& phrases) {
      for (int i=0; i<phrases.size(); ++i) {
        if (try_phrase(phrases[i])) {
          std::rotate(phrases.begin()+i, phrases.begin()+i+1, phrases.end());
          return true;
        }
      }
      return false;
    };

    if (!try_phrases(phrases)) {
      std::vector<std::string> allmove = random_default_moves();
      try_phrases(allmove);
    }
  }

  return result;
}

std::string generate_powerful_sequence(
    const std::string& hint,
    Game& game,
    const UnitLocation& start,
    const UnitLocation& goal,
    const std::vector<std::string>& phrases) {
  // Construct the abstract graph structure. 
  std::map<UnitLocation, int> known_unit_id;
  std::vector<UnitLocation> known_unit;
  auto unit_to_id = [&](const UnitLocation& u) {
    auto it = known_unit_id.find(u);
    if (it != known_unit_id.end())
      return it->second;
    int id = known_unit.size();
    known_unit_id[u] = id;
    known_unit.emplace_back(u);
    return id;
  };

  Graph graph(1);
  Vert S = unit_to_id(start);
  Vert G = unit_to_id(goal);

  std::queue<int> Q;
  Q.push(S);
  std::set<int> V;
  V.insert(S);
  while (!Q.empty()) {
    Vert v = Q.front();
    Q.pop();

    for (Game::Command c = Game::Command::E; c != Game::Command::IGNORED; ++c) {
      if (HURRY_UP_MODE)
        return hint;
      UnitLocation uu = Game::NextUnit(known_unit[v], c);
      if (uu.pivot().y() > goal.pivot().y())
        continue;
      if (game.GetBoard().IsConflicting(uu))
        continue;
      Vert u = unit_to_id(uu);
      if (graph.size() <= v) graph.resize(v+1);
      graph[v].emplace_back(c, u);
      if (V.count(u))
        continue;
      Q.push(u);
      V.insert(u);
      if (graph.size() <= u) graph.resize(u+1);
    }
  }

  std::vector<int> depth(graph.size());
  for (int v=0; v<graph.size(); ++v)
    depth[v] = known_unit[v].pivot().y();

  // Solve over the graph.
  VLOG(1) << "  Graph Generated (" << graph.size() << " nodes)";
  if (HURRY_UP_MODE)
    return hint;
  return solve_on_graph(hint, graph, depth, S, G, phrases);
}

void rewrite_main(
    const picojson::value& problem,
    picojson::value* output_entry,
    const std::vector<std::string>& phrases) {
  CHECK_EQ(problem.get("id").get<int64_t>(),
           output_entry->get("problemId").get<int64_t>());

  int seed_index = -1; {
    int64_t seed = output_entry->get("seed").get<int64_t>();
    const auto& source_seeds = problem.get("sourceSeeds").get<picojson::array>();
    for (int i=0; i<source_seeds.size(); ++i) {
      if (source_seeds[i].get<int64_t>() == seed) {
        seed_index = i;
        break;
      }
    }
    CHECK_NE(-1, seed_index);
  }

  // Original metadata.
  std::string before = output_entry->get("solution").get<std::string>();
  std::string old_tag = output_entry->get("tag").get<std::string>();
  std::string after;
  int64_t oldscore = (output_entry->contains("_score") ?
      output_entry->get("_score").get<int64_t>() : 0);

  // Initialize the game.
  GameData game_data;
  game_data.Load(problem);
  Game game;
  game.Init(&game_data, seed_index);

  // Split to subsegments.
  for (int s=0; s<before.size(); ) {
    if (HURRY_UP_MODE) {
      after += before.substr(s);
      break;
    }
    // New unit spawned.
    UnitLocation start = game.current_unit();
    for (int i=s;; ++i) {
      if (HURRY_UP_MODE)
        break;
      auto cmd = Game::Char2Command(before[i]);
      UnitLocation u = game.current_unit();
      if (game.IsLockableBy(u,cmd) || i+1==before.size()) {
        // If this is the last move for this unit, proceed to subproblem.
        VLOG(1) << (s*100/before.size()) << "% [" << before.substr(s, i-s)
            << "][" << before[i] << "]" << std::endl;
        // (Reinitialize RNG, for reproducibility.)
        g_rand = std::mt19937(178116);
        after += generate_powerful_sequence(
           before.substr(s, i-s), game, start, u, phrases);
        after += before[i];
        game.Run(cmd);
        s = i+1;
        break;
      }
      // One step ahead.
      game.Run(cmd);
    }
  }

  // Output metadata.
  int beforescore = score(before, phrases);
  int afterscore = score(after, phrases);
  LOG(INFO) << "Before: " << oldscore + beforescore;
  LOG(INFO) << "After: " << oldscore + afterscore << "(+" << afterscore << ")";
  output_entry->get("solution") = picojson::value(after);
  output_entry->get("tag") = picojson::value("rewrakkuma");
  output_entry->get("_score") = picojson::value(oldscore + afterscore - beforescore);
}

/////////////////////////////////////////////////////////////////////////////////

DEFINE_string(problem, "", "problem file");
DEFINE_string(output, "", "output file");
DEFINE_int64(id, -1, "specific id");
DEFINE_string(p,
  "Ei!,"
  "R'lyeh,"
  "yuggoth,"
  "ia! ia!,"
  "necronomiconi,"
  "yogsothoth"
  , "comma separted list of phrases");

std::vector<std::string> split(const std::string& str, char sep=',') {
  std::vector<std::string> result;
  size_t i = 0;
  while (i < str.size()) {
    size_t e = str.find(sep, i);
    if (e == std::string::npos)
      e = str.size();
    result.emplace_back(str.substr(i, e-i));
    i = e+1;
  }
  return result;
}

std::string to_lower(const std::string& s) {
  std::string r;
  for (char c: s)
    r += ('A'<=c && c<='Z' ? c-'A'+'a': c);
  return r;
}

int main(int argc, char* argv[]) {
  // Initialization.
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  FLAGS_logtostderr = true;
  install_signal_handlers();

  // Read --output json, the original solution to be modified.
  picojson::value output;
  {
    std::ifstream stream(FLAGS_output);
    stream >> output;
    CHECK(stream.good()) << picojson::get_last_error();
  }

  // Read --p option, the comma separated list of phrases.
  std::vector<std::string> phrases = split(FLAGS_p);
  for (auto& p: phrases)
    p = to_lower(p);

  // For each --output entry...
  for (auto& entry : output.get<picojson::array>()) {
    if (HURRY_UP_MODE)
      continue;
    // Load the corresponding problem.
    // If --problem points to a json file, open it.
    // Otherwise assume it to be a directory and find read problem_%d.json.
    picojson::value problem;
    if (FLAGS_problem.size()>=4 && FLAGS_problem.substr(FLAGS_problem.size()-4)=="json") {
      std::ifstream stream(FLAGS_problem);
      stream >> problem;
      CHECK(stream.good()) << picojson::get_last_error();
      rewrite_main(problem, &entry, phrases);
    } else {
      int id = entry.get("problemId").get<int64_t>();
      // id filtering.
      if (id!=178116 && (FLAGS_id==-1 || FLAGS_id==id)) {
        std::stringstream ss;
        ss << FLAGS_problem << "/problem_" << id << ".json";
        LOG(INFO) << "Problem=" << ss.str();
        try {
          std::ifstream stream(ss.str());
          stream >> problem;
          CHECK(stream.good()) << picojson::get_last_error();
          rewrite_main(problem, &entry, phrases);
        } catch (...) {
          LOG(ERROR) << "ERROR: " << id;
        }
      }
    }
  }
  std::cout << output.serialize() << std::endl;
}

