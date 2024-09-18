
.PHONY: all
all: timer rng_stream storage blink nx touch

%/app.bin:
	$(MAKE) -C $(@D) all

blink: blink/app.bin

nx: nx/app.bin

rng_stream: rng_stream/app.bin

storage: storage/app.bin

timer: timer/app.bin

touch: touch/app.bin

.PHONY: checkfmt
checkfmt:
	$(MAKE) -C nx checkfmt
	$(MAKE) -C rng_stream checkfmt
	$(MAKE) -C storage checkfmt
	$(MAKE) -C timer checkfmt
	$(MAKE) -C touch checkfmt

.PHONY: clean
clean:
	$(MAKE) -C blink clean
	$(MAKE) -C nx clean
	$(MAKE) -C rng_stream clean
	$(MAKE) -C storage clean
	$(MAKE) -C timer clean
	$(MAKE) -C touch clean
