cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0048 NEW)

project(ctk VERSION 0.1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

option(CTK_PYTHON "Build ctk python bindings?" OFF)
option(CTK_TEST_SELF "Build ctk self tests?" OFF)
option(CTK_TEST_AFL "Build ctk afl tests?" OFF)
option(CTK_TEST_LIBEEP "Build libeep/ctk comparison tests?" OFF)

if(MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /GS /W4 /permissive-")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ox /W4 /permissive-")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Ox /W4 /permissive-")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /Ox /W4 /permissive-")
	add_definitions(-D_SCL_SECURE_NO_WARNINGS)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_UNICODE)
    add_definitions(-DUNICODE)
else()
    set(CTK_CXX_FLAGS -ftrapv -pedantic -Wconversion -Wall -Wextra -Wstrict-overflow)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined,address,null")
    add_compile_options("SHELL:-fdiagnostics-color=always") # colored output in ninja
endif()


# made available if CTK_PYTHON is ON
Include(FetchContent)
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG        v2.7.1
)

# made available if CTK_TEST_LIBEEP is ON
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

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.9.2
)
FetchContent_GetProperties(spdlog)
if(NOT spdlog_POPULATED)
    FetchContent_Populate(spdlog)
    if(NOT MSVC)
        add_definitions(-fPIC)
    endif(NOT MSVC)
    add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
    if(NOT MSVC)
        remove_definitions(-fPIC)
    endif(NOT MSVC)
endif(NOT spdlog_POPULATED)


include(GNUInstallDirs)
include(GenerateExportHeader)

configure_file(ctk_version.h.in ctk_version.h)

add_library(ctk_static
    STATIC
        ctk/type_wrapper.h
        src/type_wrapper.cc
        ctk/maybe_cast.h
        src/maybe_cast.cc
        ctk/arithmetic.h
        src/arithmetic.cc
        ctk/exception.h
        src/exception.cc
        ctk/logger.h
        src/logger.cc
        ctk/compress/bit_stream.h
        src/compress/bit_stream.cc
        ctk/compress/block.h
        src/compress/block.cc
        ctk/compress/multiplex.h
        src/compress/multiplex.cc
        ctk/compress/magnitude.h
        src/compress/magnitude.cc
        ctk/compress/matrix.h
        src/compress/matrix.cc
        ctk/file/io.h
        src/file/io.cc
        ctk/file/ctk_part.h
        src/file/ctk_part.cc
        ctk/file/cnt_epoch.h
        src/file/cnt_epoch.cc
        ctk/file/cnt_reflib.h
        src/file/cnt_reflib.cc
        ctk/file/evt.h
        src/file/evt.cc
        ctk/api_compression.h
        src/api_compression.cc
        ctk/api_data.h
        src/api_data.cc
        ctk/api_reflib.h
        src/api_reflib.cc
        ctk/api_bindings.h
        src/api_bindings.cc
        ctk/api_c.h
        src/api_c.cc
)
target_include_directories(ctk_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ctk>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${date_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${spdlog_SOURCE_DIR}/include>
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
        ctk/api_bindings.h
        src/api_bindings.cc
        ctk/api_c.h
        src/api_c.cc
        ctk/logger.h
        src/logger.cc
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


if(CTK_TEST_SELF)
    include(CTest)
    add_subdirectory(test/self)
    add_test(NAME cast COMMAND test_throw_cast)
    add_test(NAME bitstream COMMAND compress_bitstream)
    add_test(NAME block COMMAND compress_block)
    add_test(NAME multiplex COMMAND compress_multiplex)
    add_test(NAME histogram COMMAND compress_histogram)
    add_test(NAME matrix COMMAND compress_matrix)
    add_test(NAME submatrix COMMAND file_submatrix)
    add_test(NAME cnt_objects COMMAND file_objects)
    add_test(NAME api_data COMMAND api_data)
    add_test(NAME api_reflib COMMAND api_reflib)
    add_test(NAME leb128 COMMAND file_leb128)
endif()

if(CTK_TEST_LIBEEP)
    FetchContent_MakeAvailable(libeep)
    add_subdirectory(${libeep_SOURCE_DIR}/trunk ${libeep_BINARY_DIR})
    include(CTest)
    add_subdirectory(test/libeep)
endif()

if(CTK_TEST_AFL)
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

