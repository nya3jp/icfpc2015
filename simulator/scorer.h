#ifndef SCORER_H_
#define SCORER_H_

#include <string>
#include <vector>

std::vector<std::string> ParsePhraseList(const std::string& phrase_list);

int MoveScore(int size, int ls, int ls_old);
int PowerScore(const std::string& solution,
               const std::vector<std::string>& phrase_list);
int PowerCount(const std::string& solution,
               const std::vector<std::string>& phrase_list);

#endif  // SCORER_H_
