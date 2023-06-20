
[![ci](https://github.com/tillitis/tkey-testapps/actions/workflows/ci.yaml/badge.svg?branch=main&event=push)](https://github.com/tillitis/tkey-testapps/actions/workflows/ci.yaml)

# Tillitis TKey Test Apps

This repository contains some test and example applications for the
[Tillitis](https://tillitis.se/) TKey USB security stick.

Client apps:

- `runtimer`: Control the `timer` device app.

Device apps:

- `rng_stream`: Outputs high quality random numbers directly. You can
  `cat` directly from the TKey device but see also
  [tkey-random-generator](https://github.com/tillitis/tkey-random-generator)
  for something more polished.
- `blink`: A minimalistic example in assembly.
- `nx`: Test program for the execution monitor.
- `timer`: Example/test app on how to use the hardware timer.
- `touch`: Example/test app for the touch sensor. Cycles between
  colours when touching.

See the [TKey Developer Handbook](https://dev.tillitis.se/) for how to
develop your own apps, how to run and debug them in the emulator or on
real hardware.

[Current list of known projects](https://dev.tillitis.se/projects/).

See [Release notes](RELEASE.md).

## Building

You have two options, either our OCI image
`ghcr.io/tillitis/tkey-builder` for use with a rootless podman setup,
or native tools. See [the Devoloper
Handbook](https://dev.tillitis.se/) for setup.

With native tools you should be able to use our build script:

```
$ ./build.sh
```

which also clones and builds the [TKey device
libraries](https://github.com/tillitis/tkey-libs) first.

If you want to do it manually, clone and build tkey-libs and
tkey-device-signer manually like this:

```
$ git clone -b v0.0.1 https://github.com/tillitis/tkey-libs
$ cd tkey-libs
$ make
```

Then go back to this directory and build everything:

```
$ make
```

If you cloned `tkey-libs` to somewhere else then the default set
`LIBDIR` to the path of the directory.

If your available `objcopy` is anything other than the default
`llvm-objcopy`, then define `OBJCOPY` to whatever they're called on
your system.

If you want to use podman and you have `make` you can run:

```
$ podman pull ghcr.io/tillitis/tkey-builder:2
$ make podman
```

or run podman directly with

```
$ podman run --rm --mount type=bind,source=$(CURDIR),target=/src --mount type=bind,source=$(CURDIR)/../tkey-libs,target=/tkey-libs -w /src -it ghcr.io/tillitis/tkey-builder:2 make -j
```

## Licenses and SPDX tags

Unless otherwise noted, the project sources are licensed under the
terms and conditions of the "GNU General Public License v2.0 only":

> Copyright Tillitis AB.
>
> These programs are free software: you can redistribute it and/or
> modify it under the terms of the GNU General Public License as
> published by the Free Software Foundation, version 2 only.
>
> These programs are distributed in the hope that it will be useful,
> but WITHOUT ANY WARRANTY; without even the implied warranty of
> MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
> General Public License for more details.

> You should have received a copy of the GNU General Public License
> along with this program. If not, see:
>
> https://www.gnu.org/licenses

See [LICENSE](LICENSE) for the full GPLv2-only license text.

External source code we have imported are isolated in their own
directories. They may be released under other licenses. This is noted
with a similar `LICENSE` file in every directory containing imported
sources.

The project uses single-line references to Unique License Identifiers
as defined by the Linux Foundation's [SPDX project](https://spdx.org/)
on its own source files, but not necessarily imported files. The line
in each individual source file identifies the license applicable to
that file.

The current set of valid, predefined SPDX identifiers can be found on
the SPDX License List at:

https://spdx.org/licenses/

All contributors must adhere to the [Developer Certificate of Origin](dco.md).
