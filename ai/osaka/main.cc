#include <gflags/gflags.h>
#include <glog/logging.h>

#include "osaka.h"
#include "../kamineko/kamineko.h"
#include "../../simulator/solver.h"

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  FLAGS_logtostderr = true;
  Osaka osaka;
  return RunSolver2(new Kamineko(&osaka), "Osaka");
}
