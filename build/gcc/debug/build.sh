#!/bin/sh

cmake -DCTK_SELF_TEST=OFF -DCTK_LIBEEP_TEST=OFF -DCTK_AFL_TEST=OFF -DCTK_PYTHON=OFF -DCMAKE_BUILD_TYPE=Debug ../../../ -G "Ninja"
cmake --build .
