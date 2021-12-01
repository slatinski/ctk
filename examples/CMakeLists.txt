add_executable(compression compression.cc)
target_include_directories(compression PRIVATE ctk_static)
target_link_libraries(compression PRIVATE ctk_static)

add_executable(file file.cc)
target_compile_definitions(file PRIVATE _FILE_OFFSET_BITS=64)
target_include_directories(file PRIVATE ctk_static)
target_link_libraries(file PRIVATE ctk_static)

add_executable(fix_offsets fix_offsets.cc)
target_compile_definitions(fix_offsets PRIVATE _FILE_OFFSET_BITS=64)
target_include_directories(fix_offsets PRIVATE ctk_static)
target_link_libraries(fix_offsets PRIVATE ctk_static)
