INCLUDE = -I../../googlelib/gflags/include \
          -I../../googlelib/glog/src \
          -I../../third_party/gtest/include
LIBS = -L../../googlelib/glog/.libs -lglog \
       -L../../googlelib/gflags/lib -lgflags \
       -lpthread
TEST_LIB = -L../../googlelib/gtest -lgtest_main -lgtest

.PHONY: clean

all: test hoge_test

test: test.cc
	g++ -o $@ $(INCLUDE) $< $(LIBS)

hoge_test: hoge_test.cc
	g++ -o $@ $(INCLUDE) $< $(TEST_LIB) $(LIBS)

clean:
	rm -rf test
