INCLUDE = -I../../googlelib/gflags/include \
          -I../../googlelib/glog/src \
          -I../../third_party/glog/src \
          -I../../third_party/gtest/include \
          -I../../third_party/picojson
LIBS = -L../../googlelib/glog/.libs -lglog \
       -L../../googlelib/gflags/lib -lgflags \
       -lpthread
TEST_LIBS = -L../googlelib/gtest -lgtest_main -lgtest

CFLAGS=-O2 -DPICOJSON_USE_INT64 --std=c++11 $(INCLUDE)

.PHONY: clean

all: greedy_solver flat_solver hasuta4 greedy_ai_2 greedy_ai_3

greedy_solver: greedy_solver.o board.o game.o solver.o scorer.o
	g++ -Wl,-rpath=$(PWD)/../../googlelib/glog/.libs $(CFLAGS) -o $@ $^ $(LIBS)

flat_solver: flat_solver.o board.o game.o solver.o scorer.o
	g++ -Wl,-rpath=$(PWD)/../../googlelib/glog/.libs $(CFLAGS) -o $@ $^ $(LIBS)

hasuta4: hasuta4.o board.o game.o solver.o scorer.o
	g++ -Wl,-rpath=$(PWD)/../../googlelib/glog/.libs $(CFLAGS) -o $@ $^ $(LIBS)

greedy_ai_3: greedy_ai_3.o board.o game.o solver.o scorer.o
	g++ -Wl,-rpath=$(PWD)/../../googlelib/glog/.libs $(CFLAGS) -o $@ $^ $(LIBS)

greedy_ai_2: greedy_ai_2.o board.o game.o solver.o scorer.o
	g++ -Wl,-rpath=$(PWD)/../../googlelib/glog/.libs $(CFLAGS) -o $@ $^ $(LIBS)

board.o: ../../simulator/board.cc
	g++ $(CFLAGS) -c -o $@ $<

game.o: ../../simulator/game.cc
	g++ $(CFLAGS) -c -o $@ $<

scorer.o: ../../simulator/scorer.cc
	g++ $(CFLAGS) -c -o $@ $<

solver.o: ../../simulator/solver.cc
	g++ $(CFLAGS) -c -o $@ $<

%_test: %_test.cc
	g++ -Wl,-rpath=$(PWD)/../../googlelib/glog/.libs $(CFLAGS) -o $@ $^ $(TEST_LIBS) $(LIBS)

%.o:%.cc
	g++ $(CFLAGS) -c -o $@ $<

clean:
	rm -rf greedy_solver *.o
