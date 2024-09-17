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
	$(MAKE) -C apps

.PHONY: runtimer
runtimer:
	go build ./cmd/runtimer

.PHONY: clean
clean:
	rm -f runtimer
	$(MAKE) -C apps clean

.PHONY: lint
lint:
	$(MAKE) -C gotools
	GOOS=linux   ./gotools/golangci-lint run
	GOOS=windows ./gotools/golangci-lint run
