.PHONY: all
all: signbench timer rng_stream blink nx touch usb_echo
	$(MAKE) -C signbench
	$(MAKE) -C timer
	$(MAKE) -C rng_stream
	$(MAKE) -C blink
	$(MAKE) -C nx
	$(MAKE) -C touch
	$(MAKE) -C usb_echo

.PHONY: checkfmt
checkfmt:
	$(MAKE) -C signbench checkfmt
	$(MAKE) -C nx checkfmt
	$(MAKE) -C rng_stream checkfmt
	$(MAKE) -C timer checkfmt
	$(MAKE) -C touch checkfmt
	$(MAKE) -C usb_echo checkfmt

.PHONY: clean
clean:
	$(MAKE) -C signbench clean
	$(MAKE) -C blink clean
	$(MAKE) -C nx clean
	$(MAKE) -C rng_stream clean
	$(MAKE) -C timer clean
	$(MAKE) -C touch clean
	$(MAKE) -C usb_echo clean
