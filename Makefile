.PHONY: all build clean setup force

all: build setup

clean:
	rm -f .build .googlelib .deps

setup:
	sudo -k
	sudo -n -l | grep -q NOPASSWD:
	sudo cgcreate -g memory:natsubate -a $(USER)
	sudo swapoff -a

force:

build: .googlelib .deps force
	$(MAKE) -C ai/simple_solvers clean
	$(MAKE) -C ai/simple_solvers
	$(MAKE) -C ai/kamineko clean
	$(MAKE) -C ai/kamineko
	$(MAKE) -C ai/duralmin clean
	$(MAKE) -C ai/duralmin
	$(MAKE) -C ai/duralstarman clean
	$(MAKE) -C ai/duralstarman
	$(MAKE) -C rewriter clean
	$(MAKE) -C rewriter

.googlelib:
	python tools/build_googlelib.py
	touch $@

.deps: install_deps.sh
	./install_deps.sh
	touch $@
