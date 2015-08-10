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

class Solver2 {
 public:
  Solver2();
  virtual ~Solver2();
  virtual void AddGame(const Game& game) = 0;
  // Returns is_finished and best_command, score.
  // TODO: should pass string** instead to avoid copy?
  virtual bool Next(std::string* best_command, int* score) = 0;
};

Solver2* ConvertS12(Solver* solver);

int RunSolver(Solver* solver, std::string solver_tag);
int RunSolver2(Solver2* solver, std::string solver_tag);

#endif  // SOLVER_H__
