.PHONY: all build clean force

all: build

clean:
	rm -f .build .googlelib .deps

force:

build: .googlelib .deps force
	sudo cgcreate -g memory:natsubate -a $(USER)
	#$(MAKE) -C ai/simple_solvers

.googlelib:
	python tools/build_googlelib.py
	touch $@

.deps: install_deps.sh
	./install_deps.sh
	touch $@
