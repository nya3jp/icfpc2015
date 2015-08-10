#include <gflags/gflags.h>
#include <glog/logging.h>

#include "duralstarman.h"
#include "../../simulator/solver.h"

DEFINE_int32(dural_width, 32, "");
DEFINE_int32(dural_depth, 1, "");

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logtostderr = true;
  KaminekoScorer base_scorer;
  DuralStarmanScorer dural(&base_scorer, FLAGS_dural_width, FLAGS_dural_depth);
return RunSolver2(new Kamineko(&dural), "DuralStarman");
}
