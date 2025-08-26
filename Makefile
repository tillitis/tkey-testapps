.PHONY: all
all: apps runtimer

DESTDIR=/
PREFIX=/usr/local
UDEVDIR=/etc/udev
destrules=$(DESTDIR)/$(UDEVDIR)/rules.d
.PHONY: install
install:
	install -Dm644 system/60-tkey.rules $(destrules)/60-tkey.rules
.PHONY: uninstall
uninstall:
	rm -f \
	$(destrules)/60-tkey.rules \
.PHONY: reload-rules
reload-rules:
	udevadm control --reload
	udevadm trigger

podman:
	podman run --rm --mount type=bind,source=$(CURDIR),target=/src --mount type=bind,source=$(CURDIR)/../tkey-libs,target=/tkey-libs -w /src -it ghcr.io/tillitis/tkey-builder:4 make -j

.PHONY: apps
apps:
	$(MAKE) -C apps/blink 
	$(MAKE) -C apps/nx
	$(MAKE) -C apps/rng_stream
	$(MAKE) -C apps/storage
	$(MAKE) -C apps/timer
	$(MAKE) -C apps/touch

.PHONY: runtimer
runtimer:
	go build ./cmd/runtimer

.PHONY: checkfmt
checkfmt:
	$(MAKE) -C apps/nx checkfmt
	$(MAKE) -C apps/rng_stream checkfmt
	$(MAKE) -C apps/storage checkfmt
	$(MAKE) -C apps/timer checkfmt
	$(MAKE) -C apps/touch checkfmt

.PHONY: clean
clean:
	rm -f runtimer
	$(MAKE) -C apps/blink clean
	$(MAKE) -C apps/nx clean
	$(MAKE) -C apps/rng_stream clean
	$(MAKE) -C apps/storage clean
	$(MAKE) -C apps/timer clean
	$(MAKE) -C apps/touch clean

.PHONY: lint
lint:
	golangci-lint run

