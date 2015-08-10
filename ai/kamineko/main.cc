#include <gflags/gflags.h>
#include <glog/logging.h>

#include "kamineko.h"
#include "../../simulator/solver.h"

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logtostderr = true;
  return RunSolver2(new Kamineko(), "Kamineko");
}
