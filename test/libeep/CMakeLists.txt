if(MSVC)
    set(LIBEEP_C_FLAGS /wd4267 /wd4244 /wd5033 /wd4838 /wd4244 /wd4477 /wd4018)
    set(LIB_FS)
else()
    set(LIBEEP_C_FLAGS -fwrapv -Wno-register -Wno-narrowing)
    set(LIB_FS stdc++fs)
endif(MSVC)

# on the fly compression
add_executable(compress_epochmux compress_epochmux.cc ../util.cc)
target_link_libraries(compress_epochmux PRIVATE ctk_static)
target_include_directories(compress_epochmux PRIVATE ctk_static ${CMAKE_SOURCE_DIR} ${libeep_SOURCE_DIR}/trunk/src)
target_compile_options(compress_epochmux PRIVATE ${LIBEEP_C_FLAGS})


# cnt files
add_executable(container_compeeg container_compeeg.cc ../util.cc)
target_link_libraries(container_compeeg PRIVATE ctk_static EepStatic ${LIB_FS})
target_include_directories(container_compeeg PRIVATE ctk_static ${CMAKE_SOURCE_DIR} ${libeep_SOURCE_DIR}/trunk/src)
target_link_directories(container_compeeg PRIVATE ${libeep_BINARY_DIR})
target_compile_options(container_compeeg PRIVATE ${LIBEEP_C_FLAGS})


