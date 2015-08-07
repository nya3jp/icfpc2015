INCLUDE = -I../googlelib/gflags/include \
          -I../googlelib/glog/src \
          -I../third_party/glog/src \
          -I../third_party/gtest/include \
          -I../third_party/picojson
LIBS = -L../googlelib/glog/.libs -lglog \
       -L../googlelib/gflags/lib -lgflags \
       -lpthread
TEST_LIB = -L../../googlelib/gtest -lgtest_main -lgtest

CFLAGS=-O2 -DPICOJSON_USE_INT64 --std=c++11 $(INCLUDE)

.PHONY: clean

all: simulator

simulator: main.o board.o
	g++ $(CFLAGS) -o $@ $^ $(LIBS)

%.o:%.cc
	g++ $(CFLAGS) -c -o $@ $<

clean:
	rm -rf simulator *.o
