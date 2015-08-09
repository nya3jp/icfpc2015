.PHONY: all clean

all: play_icfp2015

clean:
	rm -f .deps

play_icfp2015: .deps
	touch $@

.deps: install_deps.sh
	./install_deps.sh
	touch $@
