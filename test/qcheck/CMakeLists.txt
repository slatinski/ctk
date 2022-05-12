add_executable(qcheck_bitstream qcheck_bit_stream.cc)
target_link_libraries(qcheck_bitstream PRIVATE ctk_static)
target_include_directories(qcheck_bitstream PRIVATE ctk_static)
target_compile_options(qcheck_bitstream PRIVATE ${CTK_CXX_FLAGS})

add_executable(qcheck_block qcheck_block.cc)
target_link_libraries(qcheck_block PRIVATE ctk_static)
target_include_directories(qcheck_block PRIVATE ctk_static)
target_compile_options(qcheck_block PRIVATE ${CTK_CXX_FLAGS})

add_executable(qcheck_magnitude qcheck_magnitude.cc)
target_link_libraries(qcheck_magnitude PRIVATE ctk_static)
target_include_directories(qcheck_magnitude PRIVATE ctk_static)
target_compile_options(qcheck_magnitude PRIVATE ${CTK_CXX_FLAGS})

add_executable(qcheck_matrix qcheck_matrix.cc)
target_link_libraries(qcheck_matrix PRIVATE ctk_static)
target_include_directories(qcheck_matrix PRIVATE ctk_static)
target_compile_options(qcheck_matrix PRIVATE ${CTK_CXX_FLAGS})

