.PHONY: all
all: signbench timer rng_stream blink nx touch
	$(MAKE) -C signbench
	$(MAKE) -C timer
	$(MAKE) -C rng_stream
	$(MAKE) -C blink
	$(MAKE) -C nx
	$(MAKE) -C touch

.PHONY: checkfmt
checkfmt:
	$(MAKE) -C signbench checkfmt
	$(MAKE) -C nx checkfmt
	$(MAKE) -C rng_stream checkfmt
	$(MAKE) -C timer checkfmt
	$(MAKE) -C touch checkfmt

.PHONY: clean
clean:
	$(MAKE) -C signbench clean
	$(MAKE) -C blink clean
	$(MAKE) -C nx clean
	$(MAKE) -C rng_stream clean
	$(MAKE) -C timer clean
	$(MAKE) -C touch clean
