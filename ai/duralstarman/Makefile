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

all: duralstarman ds_3 ds_5 ds_7 ds_13 ds_19

duralstarman: main.o duralstarman.o board.o game.o solver.o scorer.o ai_util.o unit.o kamineko.o
	g++ -Wl,-rpath=$(PWD)/../../googlelib/glog/.libs $(CXXFLAGS) -o $@ $^ $(LIBS)

ds_3: ds_3.o duralstarman.o board.o game.o solver.o scorer.o ai_util.o unit.o kamineko.o
	g++ -Wl,-rpath=$(PWD)/../../googlelib/glog/.libs $(CXXFLAGS) -o $@ $^ $(LIBS)

ds_5: ds_5.o duralstarman.o board.o game.o solver.o scorer.o ai_util.o unit.o kamineko.o
	g++ -Wl,-rpath=$(PWD)/../../googlelib/glog/.libs $(CXXFLAGS) -o $@ $^ $(LIBS)

ds_7: ds_7.o duralstarman.o board.o game.o solver.o scorer.o ai_util.o unit.o kamineko.o
	g++ -Wl,-rpath=$(PWD)/../../googlelib/glog/.libs $(CXXFLAGS) -o $@ $^ $(LIBS)

ds_13: ds_13.o duralstarman.o board.o game.o solver.o scorer.o ai_util.o unit.o kamineko.o
	g++ -Wl,-rpath=$(PWD)/../../googlelib/glog/.libs $(CXXFLAGS) -o $@ $^ $(LIBS)

ds_19: ds_19.o duralstarman.o board.o game.o solver.o scorer.o ai_util.o unit.o kamineko.o
	g++ -Wl,-rpath=$(PWD)/../../googlelib/glog/.libs $(CXXFLAGS) -o $@ $^ $(LIBS)

ds_3.o: main.cc
	g++ -DFIXED_WIDTH=3 $(CXXFLAGS) -c -o $@ $<

ds_5.o: main.cc
	g++ -DFIXED_WIDTH=5 $(CXXFLAGS) -c -o $@ $<

ds_7.o: main.cc
	g++ -DFIXED_WIDTH=7 $(CXXFLAGS) -c -o $@ $<

ds_13.o: main.cc
	g++ -DFIXED_WIDTH=13 $(CXXFLAGS) -c -o $@ $<

ds_19.o: main.cc
	g++ -DFIXED_WIDTH=19 $(CXXFLAGS) -c -o $@ $<


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

unit.o: ../../simulator/unit.cc
	g++ $(CXXFLAGS) -c -o $@ $<

kamineko.o: ../kamineko/kamineko.cc
	g++ $(CXXFLAGS) -c -o $@ $<

%_test: %_test.cc
	g++ -Wl,-rpath=$(PWD)/../../googlelib/glog/.libs $(CXXFLAGS) -o $@ $^ $(TEST_LIBS) $(LIBS)

%.o:%.cc
	g++ $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf duralstarman *.o
