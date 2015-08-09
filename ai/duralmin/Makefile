INCLUDE = -I../../googlelib/gflags/include \
          -I../../googlelib/glog/src \
          -I../../third_party/glog/src \
          -I../../third_party/gtest/include \
          -I../../third_party/picojson
LIBS = -L../../googlelib/glog/.libs -lglog \
       -L../../googlelib/gflags/lib -lgflags \
       -lpthread
TEST_LIBS = -L../googlelib/gtest -lgtest_main -lgtest

CXXFLAGS=-O2 -DPICOJSON_USE_INT64 --std=c++11 $(INCLUDE)

.PHONY: clean

all: duralmin

duralmin: duralmin.o board.o game.o solver.o scorer.o ai_util.o
	g++ -Wl,-rpath=$(PWD)/../../googlelib/glog/.libs $(CXXFLAGS) -o $@ $^ $(LIBS)

ai_util.o: ../../simulator/ai_util.cc
	g++ $(CXXFLAGS) -c -o $@ $<

board.o: ../../simulator/board.cc
	g++ $(CXXFLAGS) -c -o $@ $<

game.o: ../../simulator/game.cc
	g++ $(CXXFLAGS) -c -o $@ $<

scorer.o: ../../simulator/scorer.cc
	g++ $(CXXFLAGS) -c -o $@ $<

solver.o: ../../simulator/solver.cc
	g++ $(CXXFLAGS) -c -o $@ $<

%_test: %_test.cc
	g++ -Wl,-rpath=$(PWD)/../../googlelib/glog/.libs $(CXXFLAGS) -o $@ $^ $(TEST_LIBS) $(LIBS)

%.o:%.cc
	g++ $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf duralmin *.o
