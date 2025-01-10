add_compile_definitions(ANYLIBS_C_TEST_ASSETS="${CMAKE_CURRENT_SOURCE_DIR}/")
add_compile_definitions(ANYLIBS_C_TEST_PLAYGROUND="${CMAKE_CURRENT_BINARY_DIR}/")

macro (create_test name deps)
    add_executable(test_${name} ${name}.c)
    target_link_libraries(test_${name} c::utest anylibs ${deps})
endmacro()

create_test(vec anylibs_src)
create_test(allocator anylibs_src)
create_test(str anylibs_src)
create_test(iter anylibs_src)
create_test(defer anylibs_src)
create_test(dl_loader anylibs_src)
create_test(fs anylibs_src)
create_test(map anylibs_src)

