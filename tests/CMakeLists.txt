macro (create_test name deps)
    add_executable(test_${name} ${name}.c)
    target_link_libraries(test_${name} c::utest anylibs ${deps})
endmacro()

create_test(array anylibs_src)

