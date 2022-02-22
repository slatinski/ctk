rem pybind11 fails because python is 64-bit
rem cmake -G "Visual Studio 16 2019" -A Win32 -S ../../ -B "build32"
cmake -DCTK_TEST_SELF=OFF -DCTK_TEST_LIBEEP=OFF -DCTK_TEST_AFL=OFF -DCTK_PYTHON=OFF -G "Visual Studio 16 2019" -A x64 -S ../../ -B "build64"

cmake --build build64 --config Release
cmake --build build64 --config Debug
