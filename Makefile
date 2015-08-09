.PHONY: all build clean setup force

all: build setup

clean:
	rm -f .build .googlelib .deps

setup:
	sudo -l | grep -q NOPASSWD:
	sudo cgcreate -g memory:natsubate -a $(USER)
	sudo swapoff -a

force:

build: .googlelib .deps force
	#$(MAKE) -C ai/simple_solvers

.googlelib:
	python tools/build_googlelib.py
	touch $@

.deps: install_deps.sh
	./install_deps.sh
	touch $@
