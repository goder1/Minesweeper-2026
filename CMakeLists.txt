cmake_minimum_required(VERSION 3.20)
project(Minesweeper2026 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    endif()
endif()

find_package(Drogon CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)
file(GLOB_RECURSE _sodium_lib "${CMAKE_BINARY_DIR}/vcpkg_installed/*/libsodium.a")
file(GLOB_RECURSE _sodium_hdr "${CMAKE_BINARY_DIR}/vcpkg_installed/*/sodium.h")

if(NOT _sodium_lib OR NOT _sodium_hdr)
    message(FATAL_ERROR "libsodium lib/header not found under vcpkg_installed")
endif()

list(GET _sodium_lib 0 SODIUM_LIBRARY)
list(GET _sodium_hdr 0 SODIUM_HEADER)
get_filename_component(SODIUM_INCLUDE_DIR "${SODIUM_HEADER}" DIRECTORY)
message(STATUS "libsodium lib: ${SODIUM_LIBRARY}")
message(STATUS "libsodium include: ${SODIUM_INCLUDE_DIR}")

set_target_properties(unofficial-sodium::sodium PROPERTIES
    IMPORTED_LOCATION "${SODIUM_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SODIUM_INCLUDE_DIR}")
find_package(jwt-cpp CONFIG REQUIRED)

add_subdirectory(src/game)
add_subdirectory(src/session)
add_subdirectory(src/app)
add_subdirectory(src/db)
add_subdirectory(src/api)

add_executable(minesweeper src/main.cpp)
target_include_directories(minesweeper PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(minesweeper PRIVATE PROJECT_WEB_DIR="${CMAKE_SOURCE_DIR}/web")
target_link_libraries(minesweeper PRIVATE
    -Wl,--whole-archive
    api_lib
    -Wl,--no-whole-archive
    Drogon::Drogon)

enable_testing()
add_subdirectory(tests/unit)
