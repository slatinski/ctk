#!/bin/sh

export CC=/usr/bin/afl-gcc
export CXX=/usr/bin/afl-g++

cmake -DCTK_SELF_TEST=OFF -DCTK_LIBEEP_TEST=OFF -DCTK_AFL_TEST=ON -DCTK_PYTHON_BINDINGS=OFF -DCMAKE_BUILD_TYPE=Debug ../../../ -G "Ninja"
AFL_HARDEN=1 ninja
