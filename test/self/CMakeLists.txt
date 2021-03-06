# consistency

add_executable(test_throw_cast test_throw_cast.cc)
target_link_libraries(test_throw_cast PRIVATE ctk_static)
target_include_directories(test_throw_cast PRIVATE ctk_static)
target_compile_options(test_throw_cast PRIVATE ${CTK_CXX_FLAGS})

add_executable(compress_bitstream compress_bitstream.cc)
target_link_libraries(compress_bitstream PRIVATE ctk_static)
target_include_directories(compress_bitstream PRIVATE ctk_static)
target_compile_options(compress_bitstream PRIVATE ${CTK_CXX_FLAGS})

add_executable(compress_block compress_block.cc)
target_link_libraries(compress_block PRIVATE ctk_static)
target_include_directories(compress_block PRIVATE ctk_static)
target_compile_options(compress_block PRIVATE ${CTK_CXX_FLAGS})

add_executable(file_leb128 leb128.cc)
target_link_libraries(file_leb128 PRIVATE ctk_static)
target_include_directories(file_leb128 PRIVATE ctk_static)
target_compile_options(file_leb128 PRIVATE ${CTK_CXX_FLAGS})

add_executable(compress_magnitude compress_magnitude.cc)
target_link_libraries(compress_magnitude PRIVATE ctk_static)
target_include_directories(compress_magnitude PRIVATE ctk_static ${CMAKE_SOURCE_DIR})
target_compile_options(compress_magnitude PRIVATE ${CTK_CXX_FLAGS})

add_executable(compress_multiplex compress_multiplex.cc)
target_link_libraries(compress_multiplex PRIVATE ctk_static)
target_include_directories(compress_multiplex PRIVATE ctk_static)
target_compile_options(compress_multiplex PRIVATE ${CTK_CXX_FLAGS})

add_executable(compress_histogram compress_histogram.cc)
target_link_libraries(compress_histogram ctk_static)
target_include_directories(compress_histogram PRIVATE ctk_static ${CMAKE_SOURCE_DIR})
target_compile_options(compress_histogram PRIVATE ${CTK_CXX_FLAGS})

add_executable(compress_matrix compress_matrix.cc)
target_link_libraries(compress_matrix PRIVATE ctk_static)
target_include_directories(compress_matrix PRIVATE ctk_static ${CMAKE_SOURCE_DIR})
target_compile_options(compress_matrix PRIVATE ${CTK_CXX_FLAGS})

add_executable(file_objects file_objects.cc)
target_link_libraries(file_objects PRIVATE ctk_static)
target_include_directories(file_objects PRIVATE ctk_static ${CMAKE_SOURCE_DIR})
target_compile_options(file_objects PRIVATE ${CTK_CXX_FLAGS})

add_executable(file_submatrix file_submatrix.cc)
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
# example content of input.txt:
# /path/to/file1.cnt
# /path/to/file2.cnt

add_executable(input_txt_magnitude input_txt_magnitude.cc ../util.cc)
target_link_libraries(input_txt_magnitude PRIVATE ctk_static)
target_include_directories(input_txt_magnitude PRIVATE ctk_static ${CMAKE_SOURCE_DIR})
target_compile_options(input_txt_magnitude PRIVATE ${CTK_CXX_FLAGS})

add_executable(input_txt_flat input_txt_flat.cc ../util.cc)
target_link_libraries(input_txt_flat PRIVATE ctk_static)
target_include_directories(input_txt_flat PRIVATE ctk_static ${CMAKE_SOURCE_DIR})
target_compile_options(input_txt_flat PRIVATE ${CTK_CXX_FLAGS})

add_executable(input_txt_abstraction input_txt_abstraction.cc ../util.cc)
target_link_libraries(input_txt_abstraction PRIVATE ctk_static)
target_include_directories(input_txt_abstraction PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_options(input_txt_abstraction PRIVATE ${CTK_CXX_FLAGS})

add_executable(input_txt_evt input_txt_evt.cc ../util.cc)
target_link_libraries(input_txt_evt PRIVATE ctk_static)
target_include_directories(input_txt_evt PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_options(input_txt_evt PRIVATE  ${CTK_CXX_FLAGS})
