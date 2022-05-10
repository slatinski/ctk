# consistency

add_executable(test_throw_cast test_throw_cast.cc)
target_link_libraries(test_throw_cast PRIVATE ctk_static)
target_include_directories(test_throw_cast PRIVATE ctk_static)
target_compile_options(test_throw_cast PRIVATE ${CTK_CXX_FLAGS})

add_executable(compress_bitstream compress_bitstream.cc)
target_link_libraries(compress_bitstream PRIVATE ctk_static)
target_include_directories(compress_bitstream PRIVATE ctk_static)
target_compile_options(compress_bitstream PRIVATE ${CTK_CXX_FLAGS})

add_executable(file_leb128 leb128.cc)
target_link_libraries(file_leb128 PRIVATE ctk_static)
target_include_directories(file_leb128 PRIVATE ctk_static)
target_compile_options(file_leb128 PRIVATE ${CTK_CXX_FLAGS})

add_executable(compress_multiplex compress_multiplex.cc)
target_link_libraries(compress_multiplex PRIVATE ctk_static)
target_include_directories(compress_multiplex PRIVATE ctk_static)
target_compile_options(compress_multiplex PRIVATE ${CTK_CXX_FLAGS})

add_executable(compress_histogram compress_histogram.cc ../util.cc)
target_link_libraries(compress_histogram ctk_static)
target_include_directories(compress_histogram PRIVATE ctk_static ${CMAKE_SOURCE_DIR})
target_compile_options(compress_histogram PRIVATE ${CTK_CXX_FLAGS})

add_executable(compress_matrix compress_matrix.cc ../util.cc)
target_link_libraries(compress_matrix PRIVATE ctk_static)
target_include_directories(compress_matrix PRIVATE ctk_static ${CMAKE_SOURCE_DIR})
target_compile_options(compress_matrix PRIVATE ${CTK_CXX_FLAGS})

add_executable(file_objects container_objects.cc ../util.cc)
target_link_libraries(file_objects PRIVATE ctk_static)
target_include_directories(file_objects PRIVATE ctk_static ${CMAKE_SOURCE_DIR})
target_compile_options(file_objects PRIVATE ${CTK_CXX_FLAGS})

add_executable(file_submatrix container_submatrix.cc ../util.cc)
target_link_libraries(file_submatrix PRIVATE ctk_static)
target_include_directories(file_submatrix PRIVATE ctk_static ${CMAKE_SOURCE_DIR})
target_compile_options(file_submatrix PRIVATE ${CTK_CXX_FLAGS})

add_executable(api_data api_data.cc)
target_link_libraries(api_data PRIVATE ctk_static)
target_include_directories(api_data PRIVATE ctk_static ${CMAKE_SOURCE_DIR})
target_compile_options(api_data PRIVATE ${CTK_CXX_FLAGS})

add_executable(api_reflib api_reflib.cc)
target_link_libraries(api_reflib PRIVATE ctk_static)
target_include_directories(api_reflib PRIVATE ctk_static ${CMAKE_SOURCE_DIR})
target_compile_options(api_reflib PRIVATE ${CTK_CXX_FLAGS})



# requires the presence of input.txt in the current working directory.
# input.txt: /path/to/file.cnt per line. everything after the first empty line is ignored.

add_executable(compress_magnitude compress_magnitude.cc ../util.cc)
target_link_libraries(compress_magnitude PRIVATE ctk_static)
target_include_directories(compress_magnitude PRIVATE ctk_static ${CMAKE_SOURCE_DIR})
target_compile_options(compress_magnitude PRIVATE ${CTK_CXX_FLAGS})

add_executable(file_flat container_flat.cc ../util.cc)
target_link_libraries(file_flat PRIVATE ctk_static)
target_include_directories(file_flat PRIVATE ctk_static ${CMAKE_SOURCE_DIR})
target_compile_options(file_flat PRIVATE ${CTK_CXX_FLAGS})

add_executable(file_abstraction container_abstraction.cc ../util.cc)
target_link_libraries(file_abstraction PRIVATE ctk_static)
target_include_directories(file_abstraction PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_options(file_abstraction PRIVATE ${CTK_CXX_FLAGS})

add_executable(file_evt container_evt.cc ../util.cc)
target_link_libraries(file_evt PRIVATE ctk_static)
target_include_directories(file_evt PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_options(file_evt PRIVATE  ${CTK_CXX_FLAGS})

add_executable(roundtrip_cnt copy_cnt_using_epoch.cc)
target_link_libraries(roundtrip_cnt PRIVATE ctk_static)
target_include_directories(roundtrip_cnt PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_options(roundtrip_cnt PRIVATE  ${CTK_CXX_FLAGS})
