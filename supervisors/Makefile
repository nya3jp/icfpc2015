INCLUDE = -I../googlelib/gflags/include \
          -I../googlelib/glog/src \
          -I../third_party/glog/src \
          -I../third_party/gtest/include \
          -I../third_party/picojson
LIBS = -L../googlelib/glog/.libs -lglog \
       -L../googlelib/gflags/lib -lgflags \
       -lpthread

CFLAGS=-O0 -g -DPICOJSON_USE_INT64 --std=c++11 $(INCLUDE)

.PHONY: all clean

all: hazuki_proxy

hazuki_proxy: hazuki_proxy.o
	g++ -Wl,-rpath=$(PWD)/../googlelib/glog/.libs $(CFLAGS) -o $@ $^ $(LIBS)

%.o:%.cc
	g++ $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o hazuki_proxy
