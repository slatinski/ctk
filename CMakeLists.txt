cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0048 NEW)

project(ctk VERSION 0.1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

option(CTK_PYTHON "Build ctk python bindings?" OFF)
option(CTK_SELF_TEST "Build ctk test suite?" OFF)
option(CTK_LIBEEP_TEST "Build lieep/ctk comparison test suite? (requires libeep)" OFF)
option(CTK_AFL_TEST "Build ctk afl test suite?" OFF)

if(MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /GS /W3 /permissive-")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ox /W3 /permissive-")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Ox /W3 /permissive-")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /Ox /W3 /permissive-")
	add_definitions(-D_SCL_SECURE_NO_WARNINGS)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_UNICODE)
    add_definitions(-DUNICODE)
else()
    set(CTK_CXX_FLAGS -ftrapv -pedantic -Wconversion -Wall -Wextra -Wstrict-overflow)
    if(NOT CTK_AFL_TEST AND NOT CTK_PYTHON)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined,address,null")
    endif()
    add_compile_options("SHELL:-fdiagnostics-color=always") # colored output in ninja
endif()


# made available if CTK_PYTHON is ON
Include(FetchContent)
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG        v2.7.1
)

# made available if CTK_LIBEEP_TEST is ON
FetchContent_Declare(
  libeep
  SVN_REPOSITORY svn://svn.code.sf.net/p/libeep/code
  SVN_REVISION   -r152
)

FetchContent_Declare(
    date
    GIT_REPOSITORY https://github.com/HowardHinnant/date.git
    GIT_TAG        v3.0.1
)
FetchContent_MakeAvailable(date)


include(GNUInstallDirs)
include(GenerateExportHeader)

configure_file(ctk_version.h.in ctk_version.h)

add_library(ctk_static
    STATIC
        ctk/type_wrapper.h
        src/type_wrapper.cc
        ctk/throw_cast.h
        ctk/arithmetic.h
        ctk/exception.h
        ctk/compress/bit_stream.h
        src/compress/bit_stream.cc
        ctk/compress/block.h
        src/compress/block.cc
        ctk/compress/multiplex.h
        ctk/compress/matrix.h
        src/compress/matrix.cc
        ctk/container/io.h
        src/container/io.cc
        ctk/container/ctk_part.h
        src/container/ctk_part.cc
        ctk/container/file_epoch.h
        src/container/file_epoch.cc
        ctk/container/file_reflib.h
        src/container/file_reflib.cc
        ctk/container/api_io.h
        src/container/api_io.cc
        ctk/evt/event_lib.h
        src/evt/event_lib.cc
        ctk/api_compression.h
        src/api_compression.cc
        ctk/api_data.h
        src/api_data.cc
        ctk/api_reflib.h
        src/api_reflib.cc
)
target_include_directories(ctk_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ctk>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${date_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ctk>
)
target_compile_options(ctk_static PRIVATE ${CTK_CXX_FLAGS})
set_target_properties(ctk_static PROPERTIES POSITION_INDEPENDENT_CODE ON) # for pybind11
#set_target_properties(ctk_static PROPERTIES PUBLIC_HEADER ctk/ctk.h)

add_library(ctk
    SHARED
        ctk/api_compression.h
        src/api_compression.cc
        ctk/api_data.h
        src/api_data.cc
        ctk/api_reflib.h
        src/api_reflib.cc
)
target_include_directories(ctk
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ctk>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ctk>
)
target_compile_options(ctk PRIVATE ${CTK_CXX_FLAGS})
target_link_libraries(ctk PRIVATE ctk_static)

# ctk_export.h
generate_export_header(ctk BASE_NAME ctk)

if(CTK_PYTHON)
    find_package(Python COMPONENTS Interpreter Development)
    FetchContent_MakeAvailable(pybind11)
    pybind11_add_module(ctkpy ffi/ctkpy.cc)
    target_link_libraries(ctkpy PRIVATE ctk_static)
endif()


if(CTK_SELF_TEST)
    include(CTest)
    add_subdirectory(test/self)
    add_test(NAME cast COMMAND test_throw_cast)
    add_test(NAME bitstream COMMAND compress_bitstream)
    add_test(NAME multiplex COMMAND compress_multiplex)
    add_test(NAME histogram COMMAND compress_histogram)
    add_test(NAME matrix COMMAND compress_matrix)
    add_test(NAME submatrix COMMAND container_submatrix)
    add_test(NAME cnt_objects COMMAND container_objects)
    add_test(NAME api_data COMMAND api_data)
    add_test(NAME leb128 COMMAND leb128)
endif()

if(CTK_LIBEEP_TEST)
    FetchContent_MakeAvailable(libeep)
    add_subdirectory(${libeep_SOURCE_DIR}/trunk ${libeep_BINARY_DIR})
    include(CTest)
    add_subdirectory(test/libeep)
    add_test(NAME compress_epochmux COMMAND compress_epochmux)
    add_test(NAME container_compeeg COMMAND container_compeeg)
endif()

if(CTK_AFL_TEST)
    add_subdirectory(test/afl)
endif()

add_subdirectory(examples)


include(CMakePackageConfigHelpers)
write_basic_package_version_file("ctkConfigVersion.cmake"
    VERSION ${ctk_Version}
    COMPATIBILITY SameMajorVersion
)
configure_package_config_file("${CMAKE_SOURCE_DIR}/cmake/ctkConfig.cmake.in" "${CMAKE_BINARY_DIR}/ctkConfig.cmake"
    INSTALL_DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/ctk
)
install(
    FILES
        "${PROJECT_BINARY_DIR}/ctkConfigVersion.cmake"
        "${PROJECT_BINARY_DIR}/ctkConfig.cmake"
        "${PROJECT_BINARY_DIR}/ctkTargets.cmake"
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/ctk
)


install(TARGETS ctk_static ctk
    EXPORT ctk
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Runtime
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Development
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT Development
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime
    #PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(EXPORT ctk
    FILE ctk.cmake
    NAMESPACE ctk::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ctk
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/ctk
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
install(FILES ${PROJECT_BINARY_DIR}/ctk_export.h ${PROJECT_BINARY_DIR}/ctk_version.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ctk
)

export(TARGETS ctk_static ctk
    NAMESPACE ctk::
    FILE ctkTargets.cmake
)
export(PACKAGE ctk)

