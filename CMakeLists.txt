cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(bustache VERSION 0.1 LANGUAGES CXX)

# option(BUSTACHE_ENABLE_TESTING "Enable testing of the bustache library." ON)

# Set the default CMAKE_BUILD_TYPE to Release.
# This should be done before the project command since the latter can set
# CMAKE_BUILD_TYPE itself (it does so for nmake).
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
    "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
endif ()

add_library(bustache
    src/format.cpp
)

find_package(Boost REQUIRED)

# Define headers for this library. PUBLIC headers are used for
# compiling the library, and will be added to consumers' build
# paths.
target_include_directories(bustache
    PUBLIC include ${Boost_INCLUDE_DIR}>
)

# If we have compiler requirements for this library, list them
# here
target_compile_features(bustache
    PUBLIC
        cxx_alias_templates
        cxx_auto_type
        cxx_decltype_auto
        cxx_user_literals
        cxx_rvalue_references
        cxx_range_for
        cxx_nullptr
        cxx_noexcept
        cxx_inline_namespaces
)

# 'make install' to the correct location
install(TARGETS bustache
    ARCHIVE  DESTINATION lib
    LIBRARY  DESTINATION lib
    RUNTIME  DESTINATION bin  # This is for Windows
)

install(DIRECTORY include/ DESTINATION include)
# if (BENCHMARK_ENABLE_TESTING)
#     enable_testing()
#     add_subdirectory(test)
# endif()
