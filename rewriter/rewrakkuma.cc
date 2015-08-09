#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <algorithm>
#include <set>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <picojson.h>

#include "../../simulator/board.h"
#include "../../simulator/game.h"
#include "../../simulator/unit.h"

// Tenuki.
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

// Tenuki.
char canonical(char c) {
  std::string group[] = {
    "p'!.03",
    "bcefy2",
    "aghij4",
    "lmno 5",
    "dqrvz1",
    "kstuwx",
  };
  for(int i=0; i<6; ++i)
    if(group[i].find(c) != std::string::npos)
      return group[i][0];
  return c;
}

bool match(char c1, char c2) {
	return canonical(c1) == canonical(c2);
}

bool match(const std::string& base, int s, const std::string& target) {
	for(int i=0; i<target.size(); ++i)
		if(s+i>=base.size() || !match(base[s+i], target[i]))
			return false;
	return true;
}

std::string solve(const std::string& cmd, const std::vector<std::string>& phrases) {
	LOG(INFO) << "Before: " << score(cmd, phrases);

	// simple greedy changer.
	// TODO dp-based changer.
	std::set<std::string> p1(phrases.begin(), phrases.end()), p2;
	std::string result;
	while(result.size() < cmd.size()) {
		bool done = false;
		if(!done)
			for(auto p: p1)
				if(match(cmd,result.size(),p)) {
					done = true;
					result += p;
					p2.insert(p);
					p1.erase(p);
					break;
				}
		if(!done)
			for(auto p: p2)
				if(match(cmd,result.size(),p)) {
					done = true;
					result += p;
					break;
				}
		if(!done)
			result += cmd[result.size()];
	}

	LOG(INFO) << "After: " << score(result, phrases);
	return result;
}

void rewrite_main(
    const picojson::value& problem,
    picojson::value* output_entry,
    const std::vector<std::string>& phrases) {
  std::string before = output_entry->get("solution").get<std::string>();
  std::string after = solve(before, phrases);
  std::string old_tag = output_entry->get("tag").get<std::string>();
  output_entry->get("solution") = picojson::value(after);
  output_entry->get("tag") = picojson::value("rewrakkuma");
}

/////////////////////////////////////////////////////////////////////////////////

DEFINE_string(problem, "", "problem file");
DEFINE_string(output, "", "output file");
DEFINE_string(p, "", "comma separted list of phrases");

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
  picojson::value output;
  {
    std::ifstream stream(FLAGS_output);
    stream >> output;
    CHECK(stream.good()) << picojson::get_last_error();
  }
  std::vector<std::string> phrases = split(FLAGS_p);

  for (auto& entry : output.get<picojson::array>()) {
    CHECK_EQ(problem.get("id").get<int64_t>(),
             entry.get("problemId").get<int64_t>());
    rewrite_main(problem, &entry, phrases);
  }
  std::cout << output.serialize() << std::endl;
}
