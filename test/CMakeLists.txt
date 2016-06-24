if (NOT TARGET Catch)
    include(ExternalProject)
    ExternalProject_Add(Catch-External
        PREFIX ${CMAKE_BINARY_DIR}/external/Catch
        GIT_REPOSITORY https://github.com/philsquared/Catch.git
        TIMEOUT 10
        UPDATE_COMMAND ${GIT_EXECUTABLE} pull
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
    )
    ExternalProject_Get_Property(Catch-External SOURCE_DIR)
    set(CATCH_INCLUDE_DIR ${SOURCE_DIR}/include CACHE INTERNAL "Path to include folder for Catch")
    add_library(Catch INTERFACE)
    add_dependencies(Catch Catch-External)
    target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})
endif()

add_executable(specs
    specs.cpp
)

target_link_libraries(specs
    ${PROJECT_NAME} Catch
)

add_test(test_specs specs)
