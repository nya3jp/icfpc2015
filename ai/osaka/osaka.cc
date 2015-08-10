#include "osaka.h"

#include <algorithm>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <glog/logging.h>

#include "../../simulator/ai_util.h"
#include "../../simulator/board.h"
#include "../../simulator/game.h"
#include "../../simulator/hexpoint.h"
#include "../../simulator/solver.h"
#include "../../simulator/unit.h"


template<typename T>
std::string DumpV(const std::vector<T>& seq) {
  std::ostringstream ofs;
  for (const auto& e : seq) {
    ofs << e << ", ";
  }
  return ofs.str();
}

Osaka::Osaka() {}
Osaka::~Osaka() {}

int64_t Osaka::Score(const Game& game, bool finished,
                     std::string* debug) {
  const Board& board(game.board());
  if (finished) {
    const int64_t height = board.height();
    const int64_t width = board.width();
    return game.score() -
      2 * (height * width * height * 100 + height * width * 2000);
  }
  std::vector<int> height(GetHeightLine(game));
  int shade = 0;
  int shade_pt = 0;
  int block = 0;
  
  for (int i = 0; i < board.width(); ++i) {
    for (int j = height[i]; j < board.height(); ++j) {
      if (!board(i, j)) {
        ++shade;
        shade_pt += (j - height[i]) * (j - height[i]);
      } else {
        ++block;
      }
    }
  }
  int64_t height_diff = GetHeightPenalty(height);
  int64_t height2 = 0;
  for (auto h : height) {
    height2 += (h - board.height()) * (h - board.height());
  }
  int64_t result = game.score()
    - height_diff * 100
    - height2 * 10
    - shade * 1800
    - block * 20
    - shade_pt;
  if (debug) {
    std::ostringstream os;
    os << "height:" << DumpV(height) 
       << "(score:" << height_diff << "," << height2 << ")"
       << " shade:" << shade << "(" << shade_pt << ")"
       << " block:" << block
       << " score:" << game.score()
       << " total:" << result;
    *debug += os.str();
  }
  return result;
}
