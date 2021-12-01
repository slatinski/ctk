#!/bin/sh

export CC=/usr/bin/clang-13
export CXX=/usr/bin/clang++-13
AR=/usr/bin/llvm-ar-13
NM=/usr/bin/llvm-nm-13
OD=/usr/bin/llvm-objdump-13
RL=/usr/bin/llvm-ranlib-13

cmake -DCTK_SELF_TEST=OFF -DCTK_LIBEEP_TEST=OFF -DCTK_AFL_TEST=OFF -DCTK_PYTHON=OFF -DCMAKE_AR=$AR -DCMAKE_NM=$NM -DCMAKE_OBJDUMP=$OD -DCMAKE_RANLIB=$RL -DCMAKE_BUILD_TYPE=Release ../../../ -G "Ninja"
cmake --build .
