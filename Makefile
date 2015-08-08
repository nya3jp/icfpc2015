all: play_icfp2015 ai/master/play_icfp2015 ai/nop/play_icfp2015

play_icfp2015:
	touch $@

ai/master/play_icfp2015:
	$(MAKE) -C ai/master play_icfp2015

ai/nop/play_icfp2015:
	$(MAKE) -C ai/nop play_icfp2015
