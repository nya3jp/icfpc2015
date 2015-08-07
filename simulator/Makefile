INCLUDE = -I../googlelib/gflags/include \
          -I../googlelib/glog/src \
          -I../third_party/glog/src \
          -I../third_party/gtest/include \
          -I../third_party/picojson
LIBS = -L../googlelib/glog/.libs -lglog \
       -L../googlelib/gflags/lib -lgflags \
       -lpthread
TEST_LIBS = -L../googlelib/gtest -lgtest_main -lgtest

CFLAGS=-O2 -DPICOJSON_USE_INT64 --std=c++11 $(INCLUDE)

.PHONY: clean

all: simulator scorer hexpoint_test

scorer: scorer.o board.o game.o
	g++ -Wl,-rpath=$(PWD)/../googlelib/glog/.libs $(CFLAGS) -o $@ $^ $(LIBS)

simulator: main.o board.o game.o
	g++ -Wl,-rpath=$(PWD)/../googlelib/glog/.libs $(CFLAGS) -o $@ $^ $(LIBS)

%_test: %_test.cc
	g++ -Wl,-rpath=$(PWD)/../googlelib/glog/.libs $(CFLAGS) -o $@ $^ $(TEST_LIBS) $(LIBS)

%.o:%.cc
	g++ $(CFLAGS) -c -o $@ $<

test: hexpoint_test rand_test
	./hexpoint_test
	./rand_test

clean:
	rm -rf simulator scorer hexpoint_test rand_test *.o
