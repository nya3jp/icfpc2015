INCLUDE = -I../googlelib/gflags/include \
          -I../googlelib/glog/src \
          -I../third_party/glog/src \
          -I../third_party/gtest/include \
          -I../third_party/picojson \
          -I../simulator
LIBS = -L../googlelib/glog/.libs -lglog \
       -L../googlelib/gflags/lib -lgflags \
       -lpthread
TEST_LIBS = -L../googlelib/gtest -lgtest_main -lgtest

CFLAGS=-O2 -DPICOJSON_USE_INT64 -g --std=c++11 $(INCLUDE)

.PHONY: clean

all: rewrakkuma rewriter01

rewrakkuma: rewrakkuma.o board.o game.o scorer.o
	g++ -Wl,-rpath=$(PWD)/../googlelib/glog/.libs $(CFLAGS) -o $@ $^ $(LIBS)

rewriter01: rewriter01.o board.o game.o scorer.o
	g++ -Wl,-rpath=$(PWD)/../googlelib/glog/.libs $(CFLAGS) -o $@ $^ $(LIBS)
board.o: ../simulator/board.cc
	g++ $(CFLAGS) -c -o $@ $<

game.o: ../simulator/game.cc
	g++ $(CFLAGS) -c -o $@ $<

scorer.o: ../simulator/scorer.cc
	g++ $(CFLAGS) -c -o $@ $<

%_test: %_test.cc
	g++ -Wl,-rpath=$(PWD)/../googlelib/glog/.libs $(CFLAGS) -o $@ $^ $(TEST_LIBS) $(LIBS)

%.o:%.cc
	g++ $(CFLAGS) -c -o $@ $<

clean:
	rm -rf rewrakkuma rewriter01 *.o
