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
	$(MAKE) -C ai/rewriter clean
	$(MAKE) -C ai/rewriter

.googlelib:
	python tools/build_googlelib.py
	touch $@

.deps: install_deps.sh
	./install_deps.sh
	touch $@
