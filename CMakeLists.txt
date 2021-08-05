cmake_minimum_required(VERSION 3.15)

project(bustache VERSION 0.1.0 LANGUAGES CXX)

option(BUSTACHE_ENABLE_TESTING "Enable testing of the bustache library." OFF)
option(BUSTACHE_USE_FMT "Use fmtlib." OFF)

message(STATUS "Started CMake for ${PROJECT_NAME} v${PROJECT_VERSION}...\n")

add_library(
  ${PROJECT_NAME}
  src/format.cpp
  src/render.cpp
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

if(BUILD_SHARED_LIBS)
  target_compile_definitions(${PROJECT_NAME} PRIVATE BUSTACHE_EXPORT INTERFACE BUSTACHE_SHARED)
endif()

# Allow usage of header files in the `src` directory, but only for utilities
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC 
    $<INSTALL_INTERFACE:include>    
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Identify and link with the specific "packages" the project uses
#find_package(package_name package_version REQUIRED package_type [other_options])
#target_link_libraries(
#  ${PROJECT_NAME}
#  PUBLIC
#    dependency1 ...
#  PRIVATE
#    dependency2 ...
#    ${PROJECT_NAME}_PROJECT_OPTIONS        
#    ${PROJECT_NAME}_PROJECT_WARNINGS        
#)
if(BUSTACHE_USE_FMT)
  find_package(fmt REQUIRED)
  target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
        fmt::fmt
  )
  add_compile_definitions(BUSTACHE_USE_FMT)
endif()

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

include(GNUInstallDirs)

# Install the library and headers.
install(
  TARGETS
    ${PROJECT_NAME}
  EXPORT
    ${PROJECT_NAME}-targets
  LIBRARY DESTINATION
    ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION
    ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION
    include
  PUBLIC_HEADER DESTINATION
    include
)

# Install version, config and target files.
install(
  EXPORT 
    ${PROJECT_NAME}-targets
  FILE
    ${PROJECT_NAME}-targets.cmake
  NAMESPACE
    ${PROJECT_NAME}::
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# Install the `include` directory
install(
  DIRECTORY
    include/${PROJECT_NAME}
  DESTINATION
    include
)

# Quick `config-version.cmake` creation
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${PROJECT_NAME}-config-version.cmake
  VERSION
    ${PROJECT_VERSION}
  COMPATIBILITY
    SameMajorVersion
)

configure_package_config_file(
  ${CMAKE_CURRENT_LIST_DIR}/cmake/${PROJECT_NAME}-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
  INSTALL_DESTINATION 
    ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# Unit testing setup
if(BUSTACHE_ENABLE_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()
