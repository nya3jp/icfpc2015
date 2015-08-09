#ifndef SOLVER_H__
#define SOLVER_H__

#include <vector>

#include "common.h"
#include "game.h"

class Solver {
public:
  Solver();
  virtual ~Solver();

  virtual std::string NextCommands(const Game& game) = 0;
};

int RunSolver(Solver* solver);

#endif  // SOLVER_H__
