#ifndef AI_UTIL_H__
#define AI_UTIL_H__

#include <vector>

#include "game.h"

std::vector<int> GetHeightLine(const Game& game);
int64_t GetHeightPenalty(const std::vector<int>& height);
int64_t GetHeightPenaltyFromGame(const Game& game);
#endif  // AI_UTIL_H__
