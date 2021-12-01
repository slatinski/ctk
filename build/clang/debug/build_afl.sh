#!/bin/sh

export CC=/usr/bin/afl-clang
export CXX=/usr/bin/afl-clang++
AR=/usr/bin/llvm-ar-13
NM=/usr/bin/llvm-nm-13
OD=/usr/bin/llvm-objdump-13
RL=/usr/bin/llvm-ranlib-13

cmake -DCTK_SELF_TEST=OFF -DCTK_LIBEEP_TEST=OFF -DCTK_AFL_TEST=ON -DCTK_PYTHON_BINDINGS=OFF -DCMAKE_AR=$AR -DCMAKE_NM=$NM -DCMAKE_OBJDUMP=$OD -DCMAKE_RANLIB=$RL -DCMAKE_BUILD_TYPE=Debug ../../../ -G "Ninja"
AFL_HARDEN=1 ninja
