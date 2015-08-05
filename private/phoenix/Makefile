INCLUDE = -I../../googlelib/gflags/include -I../../googlelib/glog/src
LIBS = -L../../googlelib/glog/.libs -lglog \
       -L../../googlelib/gflags/lib -lgflags

.PHONY: clean

test: test.cc
	g++ -o $@ $(INCLUDE) $< $(LIBS)

clean:
	rm -rf test
