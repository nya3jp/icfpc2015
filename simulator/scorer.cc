#include "scorer.h"

std::vector<std::string> ParsePhraseList(const std::string& phrase_list) {
  std::vector<std::string> result;
  size_t pos = 0;
  while (true) {
    size_t end = phrase_list.find(',', pos);
    if (end == std::string::npos) {
      result.push_back(phrase_list.substr(pos));
      break;
    }
    result.push_back(phrase_list.substr(pos, end - pos));
    pos = end + 1;
  }
  return std::move(result);
}

int MoveScore(int size, int ls, int ls_old) {
  int points = size + 100 * (1 + ls) * ls / 2;
  int line_bonus = ls_old > 1 ? ((ls_old - 1) * points / 10) : 0;
  return points + line_bonus;
}

int PowerScore(const std::string& solution,
               const std::vector<std::string>& phrase_list) {
  int result = 0;
  for (const std::string& phrase : phrase_list) {
    int reps = 0;
    for (size_t pos = 0;
         (pos = solution.find(phrase, pos)) != std::string::npos; ++pos) {
      ++reps;
    }
    int power_bonus = reps ? 300 : 0;
    int power_score = 2 * phrase.length() * reps + power_bonus;
    result += power_score;
  }
  return result;
}
