find_package(Catch REQUIRED)

add_executable(specs
    specs.cpp
)

target_include_directories(specs
    PRIVATE ${CATCH_INCLUDE_DIR}
)

target_link_libraries(specs
    ${PROJECT_NAME}
)

add_test(test_specs specs)
