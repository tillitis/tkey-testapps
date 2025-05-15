
.PHONY: all
all: timer rng_stream blink nx touch usb_echo

%/app.bin:
	$(MAKE) -C $(@D) all

blink: blink/app.bin

nx: nx/app.bin

rng_stream: rng_stream/app.bin

timer: timer/app.bin

touch: touch/app.bin

usb_echo: usb_echo/app.bin

.PHONY: checkfmt
checkfmt:
	$(MAKE) -C nx checkfmt
	$(MAKE) -C rng_stream checkfmt
	$(MAKE) -C timer checkfmt
	$(MAKE) -C touch checkfmt
	$(MAKE) -C usb_echo checkfmt

.PHONY: clean
clean:
	$(MAKE) -C blink clean
	$(MAKE) -C nx clean
	$(MAKE) -C rng_stream clean
	$(MAKE) -C timer clean
	$(MAKE) -C touch clean
	$(MAKE) -C usb_echo clean
