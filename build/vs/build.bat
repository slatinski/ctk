rem pybind11 fails because python is 64-bit
rem cmake -G "Visual Studio 16 2019" -A Win32 -S ../../ -B "build32"
cmake -DCTK_SELF_TEST=ON -DCTK_LIBEEP_TEST=OFF -DCTK_AFL_TEST=OFF -DCTK_PYTHON=ON -G "Visual Studio 16 2019" -A x64 -S ../../ -B "build64"

cmake --build build64 --config Release
cmake --build build64 --config Debug
