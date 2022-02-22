#!/bin/sh

cmake -DCTK_TEST_SELF=OFF -DCTK_TEST_LIBEEP=OFF -DCTK_TEST_AFL=OFF -DCTK_PYTHON=OFF -DCMAKE_BUILD_TYPE=Release ../../../ -G "Ninja"
cmake --build .
