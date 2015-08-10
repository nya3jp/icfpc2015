#ifndef OSAKA_H__
#define OSAKA_H__

#include <string>

#include "../../simulator/game.h"
#include "../../simulator/solver.h"

class Osaka : public GameScorer {
public:
  Osaka();
  virtual ~Osaka();

  virtual int64_t Score(const Game& game, bool finished,
                        std::string* debug);
};

#endif  // OSAKA_H__
