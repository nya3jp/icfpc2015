#include <gflags/gflags.h>
#include <glog/logging.h>

DEFINE_string(testflag, "default", "testflag");

void h() {
  LOG(FATAL) << "Failed.";
}

void g() {
  h();
}

void f() {
  g();
}

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  LOG(ERROR) << FLAGS_testflag;
  f();
  LOG(ERROR) << FLAGS_testflag;
}
