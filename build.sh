#! /bin/sh

LIBDIR=../tkey-libs

git clone -b v0.0.1 https://github.com/tillitis/tkey-libs.git ../tkey-libs

make -j -C ../tkey-libs

make -j
