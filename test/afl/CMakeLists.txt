# hostile input
# Note: disable -fsanitize=undefined and -fsanitize=address in the main CMakeLists.txt file before building

add_executable(afl_matrix_api_decode afl_matrix_api_decode.cc)
target_link_libraries(afl_matrix_api_decode PRIVATE ctk_static)
target_include_directories(afl_matrix_api_decode PRIVATE ctk_static "${CMAKE_SOURCE_DIR}")
target_compile_options(afl_matrix_api_decode PRIVATE ${CTK_CXX_FLAGS})

add_executable(afl_matrix_api_encode_decode afl_matrix_api_encode_decode.cc)
target_link_libraries(afl_matrix_api_encode_decode PRIVATE ctk_static)
target_include_directories(afl_matrix_api_encode_decode PRIVATE ctk_static "${CMAKE_SOURCE_DIR}")
target_compile_options(afl_matrix_api_encode_decode PRIVATE ${CTK_CXX_FLAGS})

add_executable(afl_file_api_read afl_file_api_read.cc)
target_link_libraries(afl_file_api_read PRIVATE ctk_static)
target_include_directories(afl_file_api_read PRIVATE ctk_static "${CMAKE_SOURCE_DIR}")
target_compile_options(afl_file_api_read PRIVATE ${CTK_CXX_FLAGS})

add_executable(afl_read_evt afl_read_evt.cc)
target_link_libraries(afl_read_evt PRIVATE ctk_static)
target_include_directories(afl_read_evt PRIVATE ctk_static "${CMAKE_SOURCE_DIR}")
target_compile_options(afl_read_evt PRIVATE ${CTK_CXX_FLAGS})

add_executable(afl_read_write_leb128 afl_read_write_leb128.cc)
target_link_libraries(afl_read_write_leb128 PRIVATE ctk_static)
target_include_directories(afl_read_write_leb128 PRIVATE ctk_static "${CMAKE_SOURCE_DIR}")
target_compile_options(afl_read_write_leb128 PRIVATE ${CTK_CXX_FLAGS})

add_executable(afl_eeph afl_eeph.cc)
target_link_libraries(afl_eeph PRIVATE ctk_static)
target_include_directories(afl_eeph PRIVATE ctk_static "${CMAKE_SOURCE_DIR}")
target_compile_options(afl_eeph PRIVATE ${CTK_CXX_FLAGS})

add_executable(afl_info afl_info.cc)
target_link_libraries(afl_info PRIVATE ctk_static)
target_include_directories(afl_info PRIVATE ctk_static "${CMAKE_SOURCE_DIR}")
target_compile_options(afl_info PRIVATE ${CTK_CXX_FLAGS})
