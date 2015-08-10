#include <cassert>
#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <algorithm>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <random>
#include <signal.h>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <picojson.h>

#include "../../simulator/board.h"
#include "../../simulator/game.h"
#include "../../simulator/unit.h"
#include "../../simulator/scorer.h"

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

template<typename T>
class six_vector {
 public:
  six_vector() : size(0) {}
  typename std::array<T, 6>::const_iterator begin() const { return data.begin(); }
  typename std::array<T, 6>::const_iterator end() const { return data.begin() + size; }
  void push_back(const T& t) { data[size++] = t; }
 private:
  std::array<T, 6> data;
  size_t size;
};

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
typedef six_vector<Edge> Edges;
typedef std::vector<Edges> Graph;

// Backward reachability... flags all the nodes reachable to goal.
std::vector<bool> calc_goal_reachability(const Graph& g, Vert goal) {
  std::vector<six_vector<int>> r(g.size());
  for (int v=0; v<g.size(); ++v)
    for (auto& cu: g[v])
      r[cu.second].push_back(v);

  std::vector<bool> visited(g.size()); visited[goal]=true;
  std::queue<Vert> Q; Q.push(goal);
  while (!Q.empty()) {
    Vert v = Q.front(); Q.pop();
    for (int u: r[v]) {
      if (!visited[u]) {
         visited[u] = true;
         Q.push(u);
      }
    }
  }
  return visited;
}

// A data structure to maintain the set of phrases.
// Determines the ordering which phrase to prefer.
class PhraseSet {
 public:
  PhraseSet(const std::vector<std::string>& phrases)
      : phrases_(phrases) {
    precompute();
    reset();
  }

  void reset() {
    seen_.assign(phrases_.size(), false);
    order_.clear();
    for (int i=0; i<phrases_.size(); ++i)
      order_.push_back(i);
    update_order();
    last_phrase_ = -1;
  }

  std::vector<int>::const_iterator begin() const {
    return order_.begin();
  }

  std::vector<int>::const_iterator end() {
    return order_.end();
  }

  std::string get(int id) {
    if (last_phrase_ == -1)
      return phrases_[id];
    return phrases_[id].substr(overwrap_[last_phrase_][id]);
  }

  void log_used(int id) {
    seen_[id] = true;
    last_phrase_ = id;
    update_order();
  }

  void log_phrase_not_used() {
    last_phrase_ = -1;
  }

 private:
  void update_order() {
    std::sort(order_.begin(), order_.end(), [&](int a, int b) {
      // prefer unseen
      if(seen_[a] != seen_[b]) return seen_[a] < seen_[b];
      const std::string& lhs = phrases_[a];
      const std::string& rhs = phrases_[b];
      if (last_phrase_ != -1) {
        // prefer less {SE,SW}.
        int sa = overwrap_south_[last_phrase_][a];
        int sb = overwrap_south_[last_phrase_][b];
        if(sa!=sb) return sa<sb;
        // prefer shorter.
        int la = phrases_[a].size() - overwrap_[last_phrase_][a];
        int lb = phrases_[b].size() - overwrap_[last_phrase_][b];
        if(la!=lb) return la<lb;
      } else {
        if (south_[a] != south_[b]) return south_[a] < south_[b];
        if (lhs.size() != rhs.size()) return lhs.size() < rhs.size();
      }
      // whatever
      return lhs < rhs;
    });
  }

  void precompute() {
    // south_
    south_.clear();
    for (const std::string& s: phrases_)
      south_.push_back(count_south(s));

    // overwrap
    overwrap_.assign(phrases_.size(), std::vector<int>(phrases_.size(), 0));
    overwrap_south_.assign(phrases_.size(), std::vector<int>(phrases_.size(), 0));
    for (int b=0; b<phrases_.size(); ++b)
    for (int a=0; a<phrases_.size(); ++a) {
      overwrap_[b][a] = count_overwrap(phrases_[b], phrases_[a]);
      overwrap_south_[b][a] = count_south(phrases_[a].substr(overwrap_[b][a]));
    }
  }

  static int count_south(const std::string& s) {
    int cnt = 0;
    for (char ch: s) {
      auto c = Game::Char2Command(ch);
      cnt += (c==Game::Command::SE || c==Game::Command::SW);
    }
    return cnt;
  }

  static int count_overwrap(const std::string& b, const std::string& a) {
    for(int l=std::min(b.size(), a.size())-1; l>=1; --l)
      if(std::equal(b.end()-l, b.end(), a.begin()))
        return l;
    return 0;
  }

 private:
  // static component
  const std::vector<std::string> phrases_;
  std::vector<int> south_;
  std::vector<std::vector<int>> overwrap_;
  std::vector<std::vector<int>> overwrap_south_;

  // dynamic component
  std::vector<int> order_;
  std::vector<bool> seen_;
  int last_phrase_;
};

std::string solve_on_graph(
    const std::string& hint,
    const Graph& Graph,
    const std::vector<int>& depth,
    Vert Start,
    Vert Goal,
    PhraseSet& phrases) {
  // preprocess.
  const std::vector<bool> reachable_to_goal =
    std::move(calc_goal_reachability(Graph, Goal));

  std::string result;
  std::vector<bool> visited(Graph.size());
  for (Vert v=0; v<visited.size(); ++ v)
    visited[v] = !reachable_to_goal[v];
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
    auto try_phrase = [&](std::string ph) {
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

    bool phrase_succeeded = false;
    for (int phi: phrases) {
      if (try_phrase(phrases.get(phi))) {
        phrases.log_used(phi);
        phrase_succeeded = true;
        break;
      }
    }
    if (!phrase_succeeded) {
      for (const auto& ph: random_default_moves())
        if (try_phrase(ph))
          break;
    }
  }

  return result;
}

// To put it in an array.
int estimate_pivot_overflow(const UnitLocation& u) {
  int xm=0x7fffffff, xM=-0x7fffffff;
  int ym=0x7fffffff, yM=-0x7fffffff;
  for (const auto& pt: u.members()) {
    xm = std::min(xm, pt.x());
    xM = std::max(xM, pt.x());
    ym = std::min(ym, pt.y());
    yM = std::max(yM, pt.y());
  }
  xm = std::min(xm, u.pivot().x());
  xM = std::max(xM, u.pivot().x());
  ym = std::min(ym, u.pivot().y());
  yM = std::max(yM, u.pivot().y());
  return (2+std::max(xM-xm, yM-ym))*2;
}

// Construct the abstract graph structure and pass to the graph-based solver.
std::string generate_powerful_sequence(
    const std::string& hint,
    Game& game,
    const UnitLocation& start,
    const UnitLocation& goal,
    PhraseSet& phrases) {
  Graph graph;

  // Unit to node ID mapping.
  const int OFF = estimate_pivot_overflow(start);
  const int W = game.board().width();
  const int H = game.board().height();
  const int SIZE = (OFF+W+OFF)*(OFF+H+OFF)*6;
  std::vector<int> known_unit_id(SIZE, -1); 
  std::vector<UnitLocation> known_unit;
  auto unit_to_id = [&](const UnitLocation& u) {
    int key = ((u.pivot().x()+OFF)*(OFF+H+OFF)+(u.pivot().y()+OFF))*6+u.angle();
    if (known_unit_id[key] != -1)
      return known_unit_id[key];
    int id = known_unit.size();
    known_unit_id[key] = id;
    known_unit.emplace_back(u);
    graph.resize(id+1);
    return id;
  };

  // Generate graph.
  std::vector<int> is_conflicting_cache(SIZE, -1);
  std::queue<int> Q; Q.push(unit_to_id(start));
  while (!Q.empty()) {
    Vert v = Q.front(); Q.pop();
    for (Game::Command c = Game::Command::E; c != Game::Command::IGNORED; ++c) {
      if (HURRY_UP_MODE)
        return hint;
      UnitLocation uu = Game::NextUnit(known_unit[v], c);
      int key = ((uu.pivot().x()+OFF)*(OFF+H+OFF)+(uu.pivot().y()+OFF))*6+uu.angle();
      bool neo = (known_unit_id[key] == -1);
      if (uu.pivot().y() > goal.pivot().y())
        continue;
      if (is_conflicting_cache[key] == -1)
         is_conflicting_cache[key] = game.GetBoard().IsConflicting(uu) ? 1 : 0;
      if (is_conflicting_cache[key])
        continue;
      Vert u = unit_to_id(uu);  // modifies known_unit_id, so after neo.
      graph[v].push_back(std::make_pair(c, u));
      if (neo) Q.push(u);
    }
  }

  // Remember depth.
  std::vector<int> depth(graph.size());
  for (int v=0; v<graph.size(); ++v)
    depth[v] = known_unit[v].pivot().y();

  // Solve over the graph.
  VLOG(1) << "  Graph Generated (" << graph.size() << " nodes)";
  if (HURRY_UP_MODE)
    return hint;
  phrases.reset();
  return solve_on_graph(hint, graph, depth, unit_to_id(start), unit_to_id(goal), phrases);
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
  const std::string before = output_entry->get("solution").get<std::string>();
  const std::string old_tag = output_entry->get("tag").get<std::string>();
  std::string after;
  const int64_t oldscore = (output_entry->contains("_score") ?
      output_entry->get("_score").get<int64_t>() : 0);
  const int beforescore = PowerScore(before, phrases);

  // Initialize the game.
  GameData game_data;
  game_data.Load(problem);
  Game game;
  game.Init(&game_data, seed_index);

  // Preprocess PhraseSet.
  PhraseSet phrase_set(phrases);

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
           before.substr(s, i-s), game, start, u, phrase_set);
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
  int afterscore = PowerScore(after, phrases);
  LOG(INFO) << "Before: " << oldscore;
  LOG(INFO) << "After: " << oldscore + afterscore - beforescore << "(+" << afterscore << ")";
  output_entry->get("solution") = picojson::value(after);
  output_entry->get("tag") = picojson::value("rewrakkuma");
  output_entry->get("_score") = picojson::value(oldscore + afterscore - beforescore);
}

/////////////////////////////////////////////////////////////////////////////////

DEFINE_string(problem, "", "problem file");
DEFINE_string(output, "", "output file");
DEFINE_int64(id, -1, "specific id");
DEFINE_string(
      p, "ei!,r'lyeh,yuggoth,ia! ia!,necronomicon,yogsothoth,planet 10,"
       "john bigboote",
       "Power phrase");

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
  auto& entries = output.get<picojson::array>();
  for (int i=0; i<entries.size(); ++i) {
    auto& entry = entries[i];
    if (HURRY_UP_MODE)
      break;
    // Load the corresponding problem.
    // If --problem points to a json file, open it.
    // Otherwise assume it to be a directory and find read problem_%d.json.
    picojson::value problem;
    if (!FLAGS_problem.empty() && FLAGS_problem.back()!='/') {
      std::ifstream stream(FLAGS_problem);
      stream >> problem;
      CHECK(stream.good()) << picojson::get_last_error();
      rewrite_main(problem, &entry, phrases);
    } else {
      int id = entry.get("problemId").get<int64_t>();
      if (FLAGS_id==-1 && id==178116) continue;
      // id filtering.
      if (FLAGS_id==-1 || FLAGS_id==id) {
        std::stringstream ss;
        ss << FLAGS_problem << "problem_" << id << ".json";
        LOG(INFO) << "Problem=" << ss.str();
        try {
          std::ifstream stream(ss.str());
          stream >> problem;
          CHECK(stream.good()) << picojson::get_last_error();
          rewrite_main(problem, &entry, phrases);
        } catch (...) {
          LOG(ERROR) << "ERROR: " << id;
        }
      } else {
        entries.erase(entries.begin() + i--);
      }
    }
  }
  std::cout << output.serialize() << std::endl;
}

