#!/bin/bash
export LD_LIBRARY_PATH=../googlelib/glog/.libs/:${LD_LIBRARY_PATH}
exec ./simulator $@
