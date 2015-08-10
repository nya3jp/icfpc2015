#ifndef DURALSTARMAN_H__
#define DURALSTARMAN_H__

#include <memory>
#include <string>
#include <vector>

#include "../../simulator/game.h"
#include "../../simulator/solver.h"
#include "../kamineko/kamineko.h"

class DuralStarmanScorer : public GameScorer {
public:
  DuralStarmanScorer(GameScorer* parent, int width, int depth);
  virtual ~DuralStarmanScorer();
  int64_t Score(const Game& game, bool finished, std::string* debug);
private:
  GameScorer* parent_;
  int width_;
  int depth_;
};

#endif  // DURALSTARMAN_H__
