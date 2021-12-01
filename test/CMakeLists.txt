#include(FetchContent)
#set(CATCH_BUILD_TESTING OFF CACHE BOOL "")
#set(CATCH_BUILD_EXAMPLES OFF CACHE BOOL "")
#FetchContent_Declare(Catch2
#  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
#  GIT_TAG        v2.11.1
#  SOURCE_SUBDIR  cmake
#)
#FetchContent_MakeAvailable(Catch2)
#find_package(Catch2)

