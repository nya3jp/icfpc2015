#include <gflags/gflags.h>
#include <glog/logging.h>

#include "duralstarman.h"
#include "../../simulator/solver.h"

DEFINE_int32(dural_width, 20, "");
DEFINE_int32(dural_depth, 2, "");

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logtostderr = true;
  KaminekoScorer base_scorer;

#ifdef FIXED_WIDTH
  FLAGS_dural_depth = FIXED_WIDTH;
  FLAGS_dural_width = 32;
#endif

  return RunSolver(
      new DuralStarmanSolver(&base_scorer, FLAGS_dural_width, FLAGS_dural_depth),
      "DuralStarman");
}
