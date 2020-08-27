find_package(Catch2 REQUIRED)

function(add_catch_test name)
    set(TEST_TARGET test_${name})
    add_executable(${TEST_TARGET}
        ${name}.cpp
    )
    target_link_libraries(${TEST_TARGET}
        ${PROJECT_NAME} Catch2::Catch2
    )
    target_compile_features(${TEST_TARGET} PUBLIC cxx_std_20)
    add_test(${TEST_TARGET} ${TEST_TARGET})
endfunction()

add_catch_test(specs)
add_catch_test(unresolved_handler)
add_catch_test(udt)
add_catch_test(inheritance)
add_catch_test(split_tag)