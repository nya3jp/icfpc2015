#ifndef KAMINEKO_H__
#define KAMINEKO_H__

#include <string>
#include <vector>

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
    bool finished;
    int64_t score;
    std::string commands;
    std::string debug;
    GamePath() {}
    GamePath(const Game& game, bool finished,
             int64_t score, const std::string& commands,
             const std::string& debug)
      : game(game), finished(finished), score(score), commands(commands),
        debug(debug) {}
  };
 private:
  std::vector<GamePath*> path_;
};

#endif  // KAMINEKO_H__
