#ifndef DURALSTARMAN_H__
#define DURALSTARMAN_H__

#include <memory>
#include <string>
#include <vector>

#include "../../simulator/game.h"
#include "../../simulator/solver.h"
#include "../kamineko/kamineko.h"

class DuralStarmanSolver : public Solver {
 public:
  DuralStarmanSolver(GameScorer* scorer, int width, int depth);
  virtual ~DuralStarmanSolver();
  virtual std::string NextCommands(const Game& game);
 private:
  GameScorer* scorer_;
  int width_;
  int depth_;
};

#endif  // DURALSTARMAN_H__
