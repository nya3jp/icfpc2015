#ifndef KAMINEKO_H__
#define KAMINEKO_H__

#include <string>

#include "../../simulator/game.h"
#include "../../simulator/solver.h"

class Kamineko: public Solver2 {
 public:
  Kamineko();
  virtual ~Kamineko();
  virtual void AddGame(const Game& game);
  virtual bool Next(std::string* best_command, int* res_score);
  struct GamePath {
    Game game;
    int64_t score;
    std::string commands;
    bool finished;
    GamePath() {}
    GamePath(const Game& game, bool finished,
             int64_t score, const std::string& commands)
      : game(game), finished(finished), score(score), commands(commands) {}
  };
 private:
  std::vector<GamePath> path_;
};

#endif  // KAMINEKO_H__
