#ifndef SOLVER_H__
#define SOLVER_H__

#include <string>
#include <vector>

#include "common.h"
#include "game.h"

class Solver {
public:
  Solver();
  virtual ~Solver();

  virtual std::string NextCommands(const Game& game) = 0;
};

int RunSolver(Solver* solver, std::string solver_tag);

#endif  // SOLVER_H__
