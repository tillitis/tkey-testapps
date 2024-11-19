.PHONY: all
all: apps runtimer tkey-mgmt

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

tkey-mgmt: apps
	cp apps/mgmt_app/app.bin cmd/tkey-mgmt/app.bin
	go build -ldflags "-w -X main.version=$(TKEY_RUNAPP_VERSION) -buildid=" -trimpath ./cmd/tkey-mgmt

.PHONY: clean
clean:
	rm -f runtimer
	rm -f tkey-mgmt
	$(MAKE) -C apps clean

.PHONY: lint
lint:
	golangci-lint run

